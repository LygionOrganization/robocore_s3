#include "BusDevices.h"

namespace {
bool integer(const JsonDocument& doc, const char* key, int low, int high) {
  return doc[key].is<int>() && doc[key].as<int>() >= low && doc[key].as<int>() <= high;
}
void failure(JsonDocument& out, const char* reason) { out["ok"] = false; out["error"] = reason; }
bool status(LYS& bus, JsonDocument& out) {
  out["bus_error"] = bus.getLastError(); out["device_status"] = bus.getState();
  bool ok = bus.getLastError() == 0 && bus.getState() == 0;
  out["ok"] = ok;
  if (!ok) out["error"] = "device did not acknowledge or reported a fault";
  return ok;
}
}

bool BusDevices::handle(const JsonDocument& in, JsonDocument& out) {
  const int type = in["T"] | -1;
  if (type != BUS_SCAN && (type < NODE_ID || type > NODE_SBUS) &&
      (type < STEPPER_ID || type > STEPPER_ENABLE) && type != ENCODER_ORIGIN &&
      type != ENCODER_READ && type != IMU_READ) return false;
  out["T"] = type; out["ok"] = false;
  if (in["rid"].is<uint32_t>()) out["rid"] = in["rid"];
  if (type == IMU_READ) {
    out["present"] = imu.present();
    OptionalQMI8658::Sample s;
    if (!imu.sample(s)) { failure(out, imu.error()); return true; }
    out["ok"] = true; out["address"] = imu.address(); out["temperature_c"] = s.temperature;
    out["sample_count"] = s.sampleCount;
    auto a = out["accel_g"].to<JsonArray>(); a.add(s.ax); a.add(s.ay); a.add(s.az);
    auto g = out["gyro_dps"].to<JsonArray>(); g.add(s.gx); g.add(s.gy); g.add(s.gz);
    return true;
  }
  BusGuard guard;
  if (type == BUS_SCAN) {
    int first = in["first"] | 0, last = in["last"] | 253;
    if ((!in["first"].isNull() && !integer(in, "first", 0, 253)) ||
        (!in["last"].isNull() && !integer(in, "last", 0, 253)) || first > last) {
      failure(out, "scan range must be 0..253"); return true;
    }
    auto ids = out["ids"].to<JsonArray>();
    for (int id = first; id <= last; ++id) {
      if (stepper_.Ping(id) == id) ids.add(id);
      delay(1); // Give the scheduler time even on an empty bus.
    }
    out["ok"] = true; out["first"] = first; out["last"] = last;
    return true;
  }
  if (!integer(in, "id", 0, 253)) { failure(out, "id must be 0..253 (no broadcast)"); return true; }
  const uint8_t id = in["id"];
  out["id"] = id;
  if (type == NODE_ID || type == STEPPER_ID) {
    if (!integer(in, "new_id", 0, 253) || in["new_id"].as<int>() == id) {
      failure(out, "new_id must be a different ID in 0..253"); return true;
    }
    const uint8_t next = in["new_id"];
    LYS& bus = type == NODE_ID ? static_cast<LYS&>(node_) : static_cast<LYS&>(stepper_);
    if (bus.Ping(next) == next) { failure(out, "new_id already responds on this bus"); return true; }
    if (bus.Ping(id) != id) { failure(out, "source device not found"); return true; }
    if (type == NODE_ID) node_.unLockEprom(id); else stepper_.unLockEprom(id);
    if (!status(bus, out)) return true;
    bus.writeByte(id, 5, next);
    // Some devices acknowledge an ID change with the new ID. Verify explicitly.
    if (bus.Ping(next) != next) {
      if (type == NODE_ID) node_.LockEprom(id); else stepper_.LockEprom(id);
      failure(out, "new ID could not be verified; scan the bus"); return true;
    }
    if (type == NODE_ID) node_.LockEprom(next); else stepper_.LockEprom(next);
    status(bus, out); out["new_id"] = next; return true;
  }
  if (type == NODE_PWM) {
    if (!integer(in,"ch",1,2) || !integer(in,"pwm",0,1000)) { failure(out,"ch=1..2, pwm=0..1000 required"); return true; }
    node_.pwmCtl(id, in["ch"], in["pwm"]); status(node_,out); return true;
  }
  if (type == NODE_RGB) {
    // Match the existing controller's GRB mapping and SDK packed color ranges.
    if (!integer(in,"ch",0,2) || !integer(in,"r",0,7) || !integer(in,"g",0,3) || !integer(in,"b",0,3)) {
      failure(out,"ch=0(all)..2, r=0..7, g/b=0..3 required"); return true;
    }
    if (in["ch"].as<int>() == 0) node_.ledAllCtrl(id,2,in["g"],in["r"],in["b"]);
    else {
      node_.ledSingleCtrl(id,in["ch"],in["g"],in["r"],in["b"]);
      if (!status(node_,out)) return true;
      node_.ledFlush(id,2);
    }
    status(node_,out); return true;
  }
  if (type == NODE_SBUS) {
    int count = node_.sbusFlush(id);
    if (!status(node_,out) || count != 33) { failure(out,"S.BUS register read failed"); return true; }
    out["sbus_status"] = node_.sbusStatus();
    auto channels = out["channels"].to<JsonArray>();
    for (int ch=1; ch<=16; ++ch) channels.add(node_.sbusGetch(ch));
    return true;
  }
  if (type == STEPPER_MODE) {
    if (!integer(in,"mode",0,2) || (in["mode"].as<int>() == 2 && !integer(in,"leader_id",0,253))) {
      failure(out,"mode=0..2 required; follower also requires leader_id=0..253"); return true;
    }
    if (in["mode"].as<int>() == 2 && in["leader_id"].as<int>() == id) { failure(out,"leader_id must differ from own ID"); return true; }
    stepper_.unLockEprom(id); if (!status(stepper_,out)) return true;
    stepper_.SetMode(id,in["mode"]);
    bool ok = status(stepper_,out);
    if (ok && in["mode"].as<int>() == 2) { stepper_.writeByte(id,7,in["leader_id"]); ok = status(stepper_,out); }
    stepper_.LockEprom(id);
    if (ok) status(stepper_,out);
    out["power_cycle_required"] = false; // Mode writes do not change phase configuration.
    return true;
  }
  if (type == STEPPER_ORIGIN || type == ENCODER_ORIGIN) {
    if (!integer(in,"pos",0,65534)) { failure(out,"pos=0..65534 required"); return true; }
    stepper_.CalibrationOfs(id,in["pos"]); status(stepper_,out); return true;
  }
  if (type == STEPPER_READ || type == ENCODER_READ) {
    int pos = stepper_.ReadPos(id);
    if (!status(stepper_,out)) return true;
    int speed = stepper_.ReadSpeed(id);
    if (!status(stepper_,out)) return true;
    out["pos"] = pos; out["speed"] = speed; return true;
  }
  if (type == STEPPER_ENABLE) {
    if (!integer(in,"enable",0,1)) { failure(out,"enable=0 or 1 required"); return true; }
    stepper_.EnableTorque(id,in["enable"]); status(stepper_,out); return true;
  }
  if (!integer(in,"speed",type==STEPPER_SPEED ? -32767 : 1,32767) ||
      !integer(in,"acc",0,254) || !integer(in,"current",0,1000) ||
      (type==STEPPER_POSITION && !integer(in,"pos",0,65534))) {
    failure(out,"invalid position/speed/acc/current register value"); return true;
  }
  // Mode and torque enable are explicit user actions, never changed implicitly.
  if (type == STEPPER_POSITION) stepper_.WritePosEx(id,in["pos"],in["speed"],in["acc"],in["current"]);
  else stepper_.WriteSpe(id,in["speed"],in["acc"],in["current"]);
  status(stepper_,out); return true;
}
