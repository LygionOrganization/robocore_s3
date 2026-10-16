// RoboCore S3 standalone test QMI-112HZ-6. Does not use RoboCoreSupport.
// Board Manager: esp32 by Espressif Systems 3.3.0; ESP32S3 Dev Module.
// USB CDC On Boot = Disabled. Keep Type-C UART connected; monitor at 115200.
// Dependencies: Arduino.h and Wire.h are included in the ESP32 board package.
// No Library Manager packages or libraries from lib/ are required.
// Reference: QST QMI8658A Datasheet Rev A, sections 5.3, 5.5, 5.6, 5.9.
// https://files.waveshare.com/upload/5/5f/QMI8658A_Datasheet_Rev_A.pdf
// d = dump registers; r = reset/reinitialize. Upload replaces firmware on board.
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

uint8_t imuAddress = 0;
bool initialized = false;
uint32_t lastCount = 0;
unsigned long lastPrint = 0;

// Read ONE explicitly addressed register; pointer write uses repeated START.
bool readByte(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(imuAddress);
  Wire.write(reg);
  uint8_t error = Wire.endTransmission(false);
  if (error) {
    Serial0.printf("I2C pointer error: addr=0x%02X reg=0x%02X code=%u\n",imuAddress,reg,error);
    return false;
  }
  if (Wire.requestFrom(imuAddress,size_t(1),true) != 1) {
    while (Wire.available()) Wire.read();
    Serial0.printf("I2C short read: reg=0x%02X\n",reg);
    return false;
  }
  int result=Wire.read();
  if (result<0) return false;
  value=uint8_t(result); return true;
}
bool writeByte(uint8_t reg,uint8_t value) {
  Wire.beginTransmission(imuAddress); Wire.write(reg); Wire.write(value);
  uint8_t error=Wire.endTransmission(true);
  if(error) Serial0.printf("I2C write error: reg=0x%02X code=%u\n",reg,error);
  return error==0;
}
bool readCount(uint32_t& value) {
  uint8_t bytes[3];
  for(uint8_t i=0;i<3;++i) if(!readByte(0x30+i,bytes[i])) return false;
  value=uint32_t(bytes[0])|(uint32_t(bytes[1])<<8)|(uint32_t(bytes[2])<<16);
  return true;
}
void dumpRegisters() {
  if(!imuAddress) { Serial0.println("No IMU address selected. Send r to retry."); return; }
  const uint8_t regs[]={0,1,2,3,4,6,8,9,0x14,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x4D};
  Serial0.printf("REGISTERS addr=0x%02X\n",imuAddress);
  for(uint8_t reg:regs) {
    uint8_t value;
    if(readByte(reg,value)) Serial0.printf("  0x%02X = 0x%02X\n",reg,value);
  }
}
bool initializeImu() {
  initialized=false; imuAddress=0;
  for(uint8_t address:{uint8_t(0x6A),uint8_t(0x6B)}) {
    Wire.beginTransmission(address);
    if(Wire.endTransmission(true)!=0) continue;
    imuAddress=address;
    uint8_t who=0,revision=0;
    bool identified=readByte(0,who)&&readByte(1,revision);
    Serial0.printf("Probe addr=0x%02X WHO_AM_I=0x%02X REVISION=0x%02X\n",address,who,revision);
    if(identified && who==0x05) break;
    imuAddress=0;
  }
  if(!imuAddress) { Serial0.println("ERROR: QMI8658 not found at 0x6A/0x6B."); return false; }
  // Clear retained modes after an ESP32-only reset; wait past the 15 ms reset time.
  if(!writeByte(0x60,0xB0)) return false;
  delay(30);
  uint8_t reset=0;
  if(!readByte(0x4D,reset)||reset!=0x80) {
    Serial0.printf("ERROR: reset incomplete: 0x4D=0x%02X, expected 0x80\n",reset);
    return false;
  }
  // Little-endian, +/-8 g, +/-512 dps, 112.1 Hz, FIFO bypass, no motion engines.
  // Test 6: only the sensor ODR changes; ranges and read timing stay unchanged.
  const uint8_t config[][2]={{8,0},{2,0x40},{0x14,0},{9,0},{3,0x26},{4,0x56},{6,0},{8,3}};
  for(const auto& setting:config) {
    if(!writeByte(setting[0],setting[1])) return false;
    uint8_t actual=0;
    if(!readByte(setting[0],actual)||actual!=setting[1]) {
      Serial0.printf("ERROR: config reg=0x%02X expected=0x%02X actual=0x%02X\n",setting[0],setting[1],actual);
      return false;
    }
  }
  delay(250); // Gyro startup and filter settling.
  if(!readCount(lastCount)) return false;
  initialized=true; dumpRegisters();
  Serial0.println("READY: tilt board; |a| should be near 1 g at rest. Rotate while reading gyro.");
  return true;
}
// Test 4: use repeated START after the register-pointer write in both read helpers.
bool readDataBurst(uint8_t* data) {
  Wire.beginTransmission(imuAddress);
  Wire.write(uint8_t(0x33));
  uint8_t error=Wire.endTransmission(false);
  if(error) {
    Serial0.printf("I2C burst pointer error: %u\n",error);
    return false;
  }
  if(Wire.requestFrom(imuAddress,size_t(14),true)!=14) {
    while(Wire.available()) Wire.read();
    Serial0.println("I2C burst short read");
    return false;
  }
  for(uint8_t i=0;i<14;++i) data[i]=uint8_t(Wire.read());
  return true;
}
void printSample() {
  uint8_t data[14]={},status=0;
  uint32_t before=lastCount,after=lastCount;
  unsigned long started=millis(); bool fresh=false;
  // Reject old data and reads crossing a sample update. Equal acceleration alone
  // is NOT evidence of stale data: a stationary sensor can return equal values.
  while(millis()-started<350) {
    if(!readByte(0x2E,status)||!readCount(before)) return;
    if(before!=lastCount) {
      if(!readDataBurst(data)) return;
      if(!readCount(after)) return;
      if(before==after) { fresh=true; break; }
    }
    delay(2);
  }
  if(!fresh) {
    Serial0.printf("ERROR: no stable NEW sample in 350ms; previous=%lu before=%lu after=%lu STATUS0=0x%02X. Send d.\n",
      (unsigned long)lastCount,(unsigned long)before,(unsigned long)after,status);
    return;
  }
  lastCount=after;
  int16_t raw[7];
  for(uint8_t i=0;i<7;++i) raw[i]=int16_t(uint16_t(data[2*i])|(uint16_t(data[2*i+1])<<8));
  float ax=raw[1]/4096.0f,ay=raw[2]/4096.0f,az=raw[3]/4096.0f;
  Serial0.printf("count=%lu status=0x%02X rawA=%d,%d,%d rawG=%d,%d,%d a[g]=%.4f,%.4f,%.4f |a|=%.3f g[dps]=%.3f,%.3f,%.3f T[C]=%.2f\n",
    (unsigned long)after,status,raw[1],raw[2],raw[3],raw[4],raw[5],raw[6],ax,ay,az,
    sqrtf(ax*ax+ay*ay+az*az),raw[4]/64.0f,raw[5]/64.0f,raw[6]/64.0f,raw[0]/256.0f);
}
void setup() {
  Serial0.begin(115200); delay(500);
  Serial0.println("\nQMI-112HZ-6 | I2C 400kHz repeated START | ODR 112.1Hz | UART0 115200 | no external IMU library | d=dump r=reset");
  Wire.begin(6,7); Wire.setClock(400000); Wire.setTimeOut(20);
  if(!initializeImu()) { Serial0.println("INIT FAILED. Send d for diagnostics, r to retry."); dumpRegisters(); }
}
void loop() {
  while(Serial0.available()) {
    int command=Serial0.read();
    if(command=='d') dumpRegisters();
    if(command=='r' && !initializeImu()) Serial0.println("INIT FAILED. Send d for diagnostics.");
  }
  if(millis()-lastPrint>=200) { lastPrint=millis(); if(initialized) printSample(); }
  delay(1);
}

