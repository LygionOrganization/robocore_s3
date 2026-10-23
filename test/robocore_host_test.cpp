#include <cassert>
#include <cmath>
#include <iostream>
#include "BusDevices.h"
#include "UsbOutput.h"

int main() {
  const std::string payload(256, 'x');
  auto started = millis();
  assert(usbOutput.write(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()) == 64);
  assert(millis() - started == 10); // Host not reading: bounded wait, no oversized core write.
  Serial.space = 64; Serial.drain = true; Serial.output.clear();
  assert(usbOutput.write(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()) == 256);
  assert(Serial.output == "\n" + payload); // Recover line framing after dropping a tail.
  Serial.connected = false; started = millis();
  assert(usbOutput.write(uint8_t('x')) == 0 && millis() == started);
  BusDevices bus;
  JsonDocument input, output;
  auto run=[&](const char* json){
    input.clear();output.clear();assert(!deserializeJson(input,json));
    assert(bus.handle(input,output));return output["ok"].as<bool>();
  };
  assert(!bus.imu.begin(Wire));
  assert(Wire.transactions==2 && Wire.writes==0);
  assert(!run("{\"T\":740,\"rid\":42}"));
  assert(output["rid"]==42 && output["error"]=="imu_not_present");
  Wire.attached=true; assert(bus.imu.begin(Wire));
  assert(run("{\"T\":740}"));
  assert(output["accel_g"][0].as<float>()==1.0f);
  assert(output["gyro_dps"][0].as<float>()==4.0f);
  assert(output["temperature_c"].as<float>()==25.0f);
  assert(output["accel_g"][1].as<float>()==-2.0f);
  assert(output["accel_g"][2].as<float>()==0.5f);
  assert(output["gyro_dps"][1].as<float>()==-10.0f);
  assert(output["gyro_dps"][2].as<float>()==2.0f);
  assert(output["sample_count"]==1);
  Wire.sampleCount=2; Wire.ax=-4096;
  assert(run("{\"T\":740}")); assert(output["accel_g"][0].as<float>()==-1.0f);
  started=millis(); assert(!run("{\"T\":740}"));
  assert(output["error"]=="imu_data_not_ready" && millis()-started==6);
  Wire.sampleCount=0xFFFFFF; assert(run("{\"T\":740}"));
  Wire.sampleCount=0; assert(run("{\"T\":740}")); // Counter rollover is a fresh sample.
  Wire.ready=false; ++Wire.sampleCount; assert(run("{\"T\":740}")); Wire.ready=true; // No status-bit gate.
  Wire.torn=true; ++Wire.sampleCount; assert(run("{\"T\":740}")); Wire.torn=false;
  assert(run("{\"T\":740}"));
  OptionalQMI8658::Sample unchanged = {1,2,3,4,5,6,7,8};
  assert(!bus.imu.sample(unchanged));
  assert(unchanged.ax==1 && unchanged.sampleCount==8);
  Wire.clock=100000; Wire.slowClockTears=true; ++Wire.sampleCount;
  assert(run("{\"T\":740}")); assert(Wire.clock==400000);
  Wire.tornReads=1; ++Wire.sampleCount;
  assert(run("{\"T\":740}")); // A single update boundary recovers in this request.
  Wire.shortBurst=true; ++Wire.sampleCount;
  assert(!run("{\"T\":740}")); assert(!Wire.locked);
  Wire.shortBurst=false; ++Wire.sampleCount; assert(run("{\"T\":740}"));
  Wire.failRead=true; assert(!run("{\"T\":740}")); Wire.failRead=false;
  Wire.ignoreConfig=true; Wire.registers[8]=0;
  assert(!bus.imu.begin(Wire)); assert(!run("{\"T\":740}"));
  assert(output["error"]=="imu_config_failed");
  Wire.ignoreConfig=false; assert(bus.imu.begin(Wire));
  Wire.failReset=true; assert(!bus.imu.begin(Wire)); assert(bus.imu.error()==std::string("imu_reset_failed"));
  Wire.failReset=false; assert(bus.imu.begin(Wire));
  Wire.failHandshake=true; assert(!bus.imu.begin(Wire));
  assert(std::string(bus.imu.error())=="imu_clock_gating_failed");
  Wire.failHandshake=false; assert(bus.imu.begin(Wire));
  BusDevices sampler;
  sampler.startImuSampling(); sampler.pollImu();
  JsonDocument readCommand, snapshot;
  deserializeJson(readCommand,"{\"T\":740}");
  sampler.handle(readCommand,snapshot);
  assert(snapshot["ok"]==true && snapshot["sample_age_ms"]==0);
  delay(101); snapshot.clear(); sampler.handle(readCommand,snapshot);
  assert(snapshot["ok"]==false && snapshot["error"]=="imu_no_fresh_sample");
  ++Wire.sampleCount; sampler.pollImu();snapshot.clear();sampler.handle(readCommand,snapshot);
  assert(snapshot["ok"]==true && snapshot["sample_count"]==Wire.sampleCount);
  assert(snapshot["sampling_mode"].isNull() && snapshot["samples_acquired"].isNull());
  assert(snapshot["sensor_restarts"].isNull() && snapshot["count_before"].isNull());
  fakeI2cBusy=true;
  snapshot.clear(); sampler.handle(readCommand,snapshot);
  assert(snapshot["ok"]==true); // Read-only snapshot does not wait for the I2C bus.
  assert(sampler.imuPresent());
  sampler.pollImu(); // Busy bus skips this poll rather than waiting indefinitely.
  fakeI2cBusy=false;
  for(unsigned i=0;i<100000;++i){
    ++Wire.sampleCount; sampler.pollImu();
    snapshot.clear(); sampler.handle(readCommand,snapshot);
    assert(snapshot["ok"]==true && snapshot["sample_count"]==Wire.sampleCount);
    delay(9);
  }
  devices[1].present=true;devices[5].present=true;
  assert(run("{\"T\":700}")); assert(output["ids"].size()==2);
  assert(!run("{\"T\":700,\"first\":20,\"last\":1}"));
  assert(!run("{\"T\":711,\"id\":254,\"ch\":1,\"pwm\":100}"));
  int writes=busWrites;
  assert(!run("{\"T\":711,\"id\":1,\"ch\":3,\"pwm\":100}"));
  assert(!run("{\"T\":711,\"id\":1,\"ch\":1,\"pwm\":1001}"));
  assert(!run("{\"T\":711,\"id\":1,\"ch\":1,\"pwm\":1.5}"));
  assert(busWrites==writes);
  assert(run("{\"T\":711,\"id\":1,\"ch\":2,\"pwm\":1000}"));
  assert(run("{\"T\":713,\"id\":1}"));assert(output["channels"].size()==16);
  assert(!run("{\"T\":720,\"id\":1,\"new_id\":5}"));
  assert(run("{\"T\":720,\"id\":1,\"new_id\":2}"));assert(devices[2].present&&!devices[1].present);
  assert(run("{\"T\":721,\"id\":2,\"mode\":1}"));
  assert(devices[2].mode==1 && output["power_cycle_required"]==false);
  assert(run("{\"T\":721,\"id\":2,\"mode\":0}"));
  assert(devices[2].mode==0 && output["power_cycle_required"]==false);
  assert(!run("{\"T\":721,\"id\":2,\"mode\":2}"));
  assert(!run("{\"T\":721,\"id\":2,\"mode\":2,\"leader_id\":2}"));
  assert(run("{\"T\":721,\"id\":2,\"mode\":2,\"leader_id\":5}"));assert(devices[2].leader==5);
  assert(!run("{\"T\":725,\"id\":2,\"speed\":-32768,\"acc\":10,\"current\":150}"));
  assert(run("{\"T\":725,\"id\":2,\"speed\":-1,\"acc\":10,\"current\":150}"));
  assert(run("{\"T\":732,\"id\":2}"));assert(output["speed"]==-1); // Valid negative speed, not a transport error.
  devices[2].enabled=1; devices[2].status=16;
  writes=busWrites;
  assert(run("{\"T\":723,\"id\":2}"));
  assert(output["device_fault"]==false && output["device_status"]==0 && output["reply_status"]==16);
  assert(output["warning"].isNull());
  devices[2].driverStatus=16;
  assert(run("{\"T\":723,\"id\":2}"));
  assert(output["current_limited"]==true && output["device_fault"]==false && output["warning"].isNull());
  assert(run("{\"T\":725,\"id\":2,\"speed\":0,\"acc\":10,\"current\":150}"));
  assert(output["ok"]==true && output["warning"].isNull());
  writes=busWrites; // Explicit stop command above writes; subsequent reads must not.
  devices[2].speed=-1;
  devices[2].driverStatus=4;
  assert(run("{\"T\":723,\"id\":2}"));
  assert(output["device_fault"]==true && !output["warning"].isNull());
  devices[2].driverStatus=0;
  assert(output["speed"]==-1 && output["pos"]==devices[2].pos);
  assert(devices[2].enabled==1 && busWrites==writes);
  failSpeedRead=true;
  assert(!run("{\"T\":723,\"id\":2}"));
  assert(output["error"]=="speed read communication failed" && output["bus_error"]==1);
  assert(output["pos"].isNull() && output["speed"].isNull());
  failSpeedRead=false;
  devices[2].status=0;
  int reads=speedReads;
  assert(!run("{\"T\":732,\"id\":3}")); assert(speedReads==reads);
  assert(!run("{\"T\":730,\"id\":2,\"pos\":65535}"));
  assert(run("{\"T\":730,\"id\":2,\"pos\":65534}"));assert(devices[2].pos==65534);
  devices[200].present=true;
  ttlData[1]=0x38; // quaternion x=0.5, y=z=0
  ttlData[6]=1;ttlData[7]=0x80; // gyro x=-1, not -32767
  ttlData[9]=0xff;ttlData[10]=0x7f;
  ttlData[15]=2;ttlData[16]=0x80;
  assert(run(R"({"T":751,"id":200,"rid":123})"));
  assert(output["rid"]==123 && output["gyro_raw"][0]==-1 && output["gyro_raw"][1]==32767);
  assert(output["accel_raw"][0]==-2);
  assert(std::abs(output["quaternion_xyzw"][3].as<float>()-std::sqrt(.75f))<.0001f);
  ttlData[1]=0x7c;assert(!run(R"({"T":751,"id":200})"));
  assert(output["quaternion_xyzw"].isNull());ttlData[1]=0x38;
  ttlShort=true;assert(!run(R"({"T":751,"id":200})"));ttlShort=false;
  assert(output["accel_raw"].isNull());
  assert(!run(R"({"T":751,"id":199})"));
  devices[200].status=4;assert(!run(R"({"T":751,"id":200})"));devices[200].status=0;
  writes=busWrites;
  assert(!run(R"({"T":750,"id":254,"new_id":201})"));
  assert(!run(R"({"T":750,"id":200,"new_id":200})"));
  assert(!run(R"({"T":750,"id":200,"new_id":2})"));
  assert(!run(R"({"T":750,"id":200,"new_id":1.5})"));
  assert(busWrites==writes);
  assert(run(R"({"T":750,"id":200,"new_id":201})"));
  assert(devices[201].present&&!devices[200].present&&ttlLock==1);
  ttlLockFail=true;
  assert(!run(R"({"T":750,"id":201,"new_id":202})"));ttlLockFail=false;
  std::cout<<"PASS: optional IMU absent/read/failure, scan, validation, ID, live mode switches, follower, signed feedback\n";
}
