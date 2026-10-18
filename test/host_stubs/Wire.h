#pragma once
#include "Arduino.h"
#include <vector>
class TwoWire {
 public:
  bool attached = false, failRead = false;
  bool ready = true, ignoreConfig = false, torn = false, failReset = false;
  bool locked=false, commandDone=false, failHandshake=false, shortBurst=false;
  uint32_t consumed=0xFFFFFFFF, lockedCount=0;
  uint32_t clock = 100000;
  unsigned tornReads = 0;
  bool slowClockTears = false;
  void setClock(uint32_t hz) { clock = hz; }
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
    if(tx.size()>1 && reg==0x0A && !failHandshake && (registers[9]&0x80)) commandDone=tx[1]!=0;
    if(tx.size()>1 && reg==0x60) { locked=false; consumed=0xFFFFFFFF; commandDone=false; }
    return 0;
  }
  size_t requestFrom(uint8_t, size_t size, bool = true) {
    index=0; rx.clear(); if(failRead) return 0;
    rx.resize(size,0);
    if(reg==0) rx[0]=5;
    else if(reg==0x2D) {
      if((registers[8]&0x80) && sampleCount!=consumed && !locked) {locked=true; lockedCount=sampleCount;}
      rx[0]=(commandDone?0x80:0)|(locked?3:0);
    }
    else if(reg==0x2E) rx[0]=ready ? 3 : 0;
    else if(reg>=0x30 && reg<=0x32) {
      // Independently addressed timestamp bytes, as on the tested sensor.
      if(size != 1) return 0;
      rx[0]=uint8_t((locked?lockedCount:sampleCount) >> (8*(reg-0x30)));
    } else if(reg==0x33 && size==14) {
      if(shortBurst) {rx.resize(4);return 4;}
      const int16_t raw[]={6400,ax,-8192,2048,256,-640,128};
      for(size_t i=0;i<7;++i){rx[2*i]=uint8_t(raw[i]);rx[2*i+1]=uint8_t(uint16_t(raw[i])>>8);}
      if(torn || (slowClockTears && clock < 400000) || tornReads) { ++sampleCount; if(tornReads) --tornReads; }
      consumed=lockedCount;locked=false;
    } else if(reg==0x40) {consumed=lockedCount;locked=false;}
    else if(size==1) rx[0]=registers[reg];
    return size;
  }
  int available() { return int(rx.size()-index); }
  int read() { return rx[index++]; }
};
inline TwoWire Wire;
