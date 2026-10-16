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
  assert(output["sample_count"]==1);
  Wire.sampleCount=2; Wire.ax=-4096;
  assert(run("{\"T\":740}")); assert(output["accel_g"][0].as<float>()==-1.0f);
  started=millis(); assert(!run("{\"T\":740}"));
  assert(output["error"]=="imu_data_timeout" && millis()-started==300);
  Wire.sampleCount=0xFFFFFF; assert(run("{\"T\":740}"));
  Wire.sampleCount=0; assert(run("{\"T\":740}")); // Counter rollover is a fresh sample.
  Wire.ready=false; assert(!run("{\"T\":740}")); Wire.ready=true;
  Wire.torn=true; ++Wire.sampleCount; assert(!run("{\"T\":740}")); Wire.torn=false;
  assert(run("{\"T\":740}"));
  Wire.failRead=true; assert(!run("{\"T\":740}")); Wire.failRead=false;
  Wire.ignoreConfig=true; Wire.registers[8]=0;
  assert(!bus.imu.begin(Wire)); assert(!run("{\"T\":740}"));
  assert(output["error"]=="imu_config_failed");
  Wire.ignoreConfig=false; assert(bus.imu.begin(Wire));
  Wire.failReset=true; assert(!bus.imu.begin(Wire)); assert(bus.imu.error()==std::string("imu_reset_failed"));
  Wire.failReset=false; assert(bus.imu.begin(Wire));
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
  int reads=speedReads;
  assert(!run("{\"T\":732,\"id\":3}")); assert(speedReads==reads);
  assert(!run("{\"T\":730,\"id\":2,\"pos\":65535}"));
  assert(run("{\"T\":730,\"id\":2,\"pos\":65534}"));assert(devices[2].pos==65534);
  std::cout<<"PASS: optional IMU absent/read/failure, scan, validation, ID, live mode switches, follower, signed feedback\n";
}
