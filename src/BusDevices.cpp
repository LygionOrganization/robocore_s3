#include "BusDevices.h"
#include "I2cGuard.h"
#include "TtlImu.h"
#include <cstring>

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
// Stepper acknowledgement status includes operating conditions such as current
// limiting. A nonzero byte alone does not mean that a command was rejected.
bool status(TTLSDClass& bus, JsonDocument& out) {
  out["bus_error"] = bus.getLastError(); out["reply_status"] = bus.getState();
  const bool ok = bus.getLastError() == 0;
  out["ok"] = ok;
  if (!ok) out["error"] = "stepper communication failed";
  return ok;
}
}

BusDevices::ImuSnapshot BusDevices::snapshot() {
  std::lock_guard<std::mutex> lock(snapshotMutex_);
  return snapshot_;
}
void BusDevices::publishSnapshot() {
  ImuSnapshot value;
  value.sample=imuCache_; value.present=imu.present(); value.valid=cachedImu_;
  value.address=imu.address(); value.updated=imuUpdated_;
  value.error=imu.error();
  // Only fixed-size copy under this lock: never I2C, JSON, delay or network I/O.
  std::lock_guard<std::mutex> lock(snapshotMutex_);
  snapshot_=value;
}
bool BusDevices::imuPresent() {
  if (backgroundImu_) return snapshot().present;
  I2cGuard guard;
  return guard.acquired && imu.present();
}
void BusDevices::startImuSampling() {
  backgroundImu_ = true;
  cachedImu_ = false;
  {
    I2cGuard guard;
    if (guard.acquired) { Wire.setClock(400000); imu.begin(Wire); }
  }
  imuRestarted_ = millis();
  publishSnapshot();
}
void BusDevices::pollImu() {
  {
    I2cGuard guard;
    if (!guard.acquired) return;
    OptionalQMI8658::Sample value;
    if (imu.sample(value)) {
      imuCache_ = value; imuUpdated_ = millis(); cachedImu_ = true;
    }
    if (uint32_t(millis() - imuRestarted_) >= 5000 &&
        (!cachedImu_ || uint32_t(millis() - imuUpdated_) > 2000)) {
      cachedImu_ = false;
      imu.begin(Wire); imuRestarted_ = millis();
    }
  }
  publishSnapshot();
}

bool BusDevices::handle(const JsonDocument& in, JsonDocument& out) {
  const int type = in["T"] | -1;
  if (type != BUS_SCAN && (type < NODE_ID || type > NODE_SBUS) &&
      (type < STEPPER_ID || type > STEPPER_ENABLE) && type != ENCODER_ORIGIN &&
      type != ENCODER_READ && type != IMU_READ && type != TTL_IMU_ID &&
      type != TTL_IMU_READ) return false;
  out["T"] = type; out["ok"] = false;
  if (in["rid"].is<uint32_t>()) out["rid"] = in["rid"];
  if (type == IMU_READ) {
    OptionalQMI8658::Sample s;
    if (backgroundImu_) {
      const ImuSnapshot value = snapshot();
      out["present"] = value.present;
      const uint32_t age = millis() - value.updated;
      out["sample_age_ms"] = value.valid ? int32_t(age) : -1;
      if (!value.valid || age > 100) {
        failure(out,value.present ? "imu_no_fresh_sample" : value.error);
        return true;
      }
      s=value.sample; out["address"] = value.address;
    } else {
      I2cGuard guard;
      if (!guard.acquired) { failure(out,"imu_bus_busy"); return true; }
      out["present"] = imu.present();
      Wire.setClock(400000);
      bool fresh=false;
      for(unsigned attempt=0;attempt<4;++attempt) {
        if(imu.sample(s)){fresh=true;break;}
        if(std::strcmp(imu.error(),"imu_data_not_ready")!=0)break;
        if(attempt<3)delay(2);
      }
      if(!fresh){failure(out,imu.error());return true;}
      out["address"] = imu.address();
    }
    out["ok"] = true; out["temperature_c"] = s.temperature;
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
  if (type == TTL_IMU_READ) {
    uint8_t data[TtlImu::FeedbackSize] = {};
    const int count = ttlImu_.Read(id, TtlImu::Feedback, data, sizeof(data));
    if (!status(ttlImu_, out) || count != sizeof(data)) {
      failure(out, "ttl_imu_read_failed"); return true;
    }
    const float x = TtlImu::halfFloat(data), y = TtlImu::halfFloat(data + 2),
                z = TtlImu::halfFloat(data + 4);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
      failure(out, "ttl_imu_invalid_quaternion"); return true;
    }
    const float remainder = 1.f - x*x - y*y - z*z;
    auto q = out["quaternion_xyzw"].to<JsonArray>();
    q.add(x); q.add(y); q.add(z); q.add(remainder > 0 ? std::sqrt(remainder) : 0.f);
    auto g = out["gyro_raw"].to<JsonArray>();
    auto a = out["accel_raw"].to<JsonArray>();
    for (unsigned axis = 0; axis < 3; ++axis) {
      g.add(TtlImu::signedMagnitude(data + 6 + axis*3));
      a.add(TtlImu::signedMagnitude(data + 15 + axis*3));
    }
    // The SDK exposes no sample counter: this is a fresh bus reply, not proof
    // that the sensor has advanced its internal sample.
    out["ok"] = true; return true;
  }
  if (type == TTL_IMU_ID) {
    if (!integer(in, "new_id", 0, 253) || in["new_id"].as<int>() == id) {
      failure(out, "new_id must be a different ID in 0..253"); return true;
    }
    const uint8_t next = in["new_id"];
    if (ttlImu_.Ping(next) == next) { failure(out, "new_id already responds on this bus"); return true; }
    if (ttlImu_.Ping(id) != id) { failure(out, "source device not found"); return true; }
    ttlImu_.writeByte(id, TtlImu::Lock, 0);
    if (!status(ttlImu_, out)) return true;
    ttlImu_.writeByte(id, TtlImu::Id, next);
    // ID writes can acknowledge under the new ID. Read back instead of retrying.
    if (ttlImu_.readByte(next, TtlImu::Id) != next || !status(ttlImu_, out)) {
      ttlImu_.writeByte(id, TtlImu::Lock, 1);
      ttlImu_.writeByte(next, TtlImu::Lock, 1);
      failure(out, "new ID could not be verified; scan the bus"); return true;
    }
    ttlImu_.writeByte(next, TtlImu::Lock, 1);
    if (!status(ttlImu_, out)) return true;
    if (ttlImu_.readByte(next, TtlImu::Lock) != 1 || !status(ttlImu_, out)) {
      failure(out, "ttl_imu_lock_verification_failed"); return true;
    }
    out["new_id"] = next; return true;
  }
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
    out["bus_error"] = stepper_.getLastError();
    if (stepper_.getLastError() != 0) { failure(out,"position read communication failed"); return true; }
    const uint8_t positionStatus = uint8_t(stepper_.getState());
    int speed = stepper_.ReadSpeed(id);
    out["bus_error"] = stepper_.getLastError();
    if (stepper_.getLastError() != 0) { failure(out,"speed read communication failed"); return true; }
    // Nonzero device status does not invalidate a correctly received payload.
    // Combine reply flags; actual driver faults are read separately below.
    const uint8_t deviceStatus = uint8_t(positionStatus | stepper_.getState());
    out["reply_status"] = deviceStatus; // Raw protocol byte; not a fault-register value.
    if (type == STEPPER_READ) {
      const int driverStatus = stepper_.readByte(id, 0x41);
      out["bus_error"] = stepper_.getLastError();
      if (driverStatus < 0 || stepper_.getLastError() != 0) {
        failure(out,"driver status register read failed"); return true;
      }
      out["device_status"] = driverStatus;
      // Decode the actual driver register, not the acknowledgement status byte.
      out["device_fault"] = (driverStatus & 0x0D) != 0; // voltage, overheat, overcurrent
      out["current_limited"] = (driverStatus & 0x10) != 0;
      out["limit_active"] = (driverStatus & 0xA0) != 0;
      out["target_out_of_range"] = (driverStatus & 0x42) != 0;
      if (driverStatus & 0x0D) out["warning"] = "driver voltage, temperature or overcurrent fault";
    }
    out["ok"] = true; out["pos"] = pos; out["speed"] = speed; return true;
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
