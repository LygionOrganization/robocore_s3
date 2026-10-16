#pragma once
#include "Arduino.h"
#include <vector>
class TwoWire {
 public:
  bool attached = false, failRead = false;
  bool ready = true, ignoreConfig = false, torn = false, failReset = false;
  uint32_t sampleCount = 1;
  int16_t ax = 4096;
  uint8_t registers[128] = {};
  unsigned transactions = 0, writes = 0;
  uint8_t address = 0, reg = 0;
  std::vector<uint8_t> tx, rx;
  size_t index = 0;
  void beginTransmission(uint8_t a) { address=a; tx.clear(); ++transactions; }
  void write(uint8_t byte) { tx.push_back(byte); }
  int endTransmission(bool = true) {
    if (!attached || address != 0x6B) return 2;
    reg=tx[0]; if(tx.size()>1) { ++writes; if(!ignoreConfig) registers[reg]=tx[1]; }
    if(tx.size()>1 && reg==0x60 && tx[1]==0xB0) {
      for(auto& value:registers) value=0;
      registers[0x4D]=failReset ? 0 : 0x80;
    }
    return 0;
  }
  size_t requestFrom(uint8_t, size_t size) {
    index=0; rx.clear(); if(failRead) return 0;
    rx.resize(size,0);
    if(reg==0) rx[0]=5;
    else if(reg==0x2E) rx[0]=ready ? 3 : 0;
    else if(reg==0x30) {
      uint32_t count=sampleCount+(torn && size==3 ? 1 : 0);
      rx[0]=uint8_t(count);rx[1]=uint8_t(count>>8);rx[2]=uint8_t(count>>16);
      if(size==17) { rx[4]=25;rx[5]=ax&255;rx[6]=uint16_t(ax)>>8;rx[12]=1; }
    } else if(size==1) rx[0]=registers[reg];
    return size;
  }
  int available() { return int(rx.size()-index); }
  int read() { return rx[index++]; }
};
inline TwoWire Wire;
