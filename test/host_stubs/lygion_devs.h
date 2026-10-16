#pragma once
#include "Arduino.h"
#include <array>
struct FakeDevice { bool present=false; int mode=0, leader=0, pos=2048, speed=-1, enabled=0; };
inline std::array<FakeDevice,254> devices;
inline int busWrites=0, speedReads=0;
class LYS {
 public:
  HardwareSerial* pSerial=nullptr;
  int error=0, fault=0;
  int getLastError() { return error; }
  int getState() { return fault; }
  bool check(int id) { error=id<0 || id>253 || !devices[id].present; return error==0; }
  int Ping(int id) { return check(id)?id:-1; }
  int writeByte(int id,int reg,int value) {
    ++busWrites;if(!check(id))return 0;
    if(reg==5){devices[value]=devices[id];devices[id].present=false;}
    if(reg==7)devices[id].leader=value;
    return 1;
  }
};
class NodeClass: public LYS {
 public:
  int unLockEprom(int id){return writeByte(id,51,0);}
  int LockEprom(int id){return writeByte(id,51,1);}
  int pwmCtl(int id,int,int value){return writeByte(id,34,value);}
  int ledAllCtrl(int id,int,int,int,int){return writeByte(id,42,1);}
  int ledSingleCtrl(int id,int,int,int,int){return writeByte(id,43,1);}
  int ledFlush(int id,int){return writeByte(id,42,1);}
  int sbusFlush(int id){return check(id)?33:0;}
  int sbusStatus(){return 0;}
  int sbusGetch(int ch){return 992+ch;}
};
class TTLSDClass: public LYS {
 public:
  int unLockEprom(int id){if(check(id))devices[id].enabled=0;return writeByte(id,55,0);}
  int LockEprom(int id){return writeByte(id,55,1);}
  int SetMode(int id,int value){if(check(id))devices[id].mode=value;return writeByte(id,33,value);}
  int CalibrationOfs(int id,int value){if(check(id))devices[id].pos=value;return writeByte(id,42,value);}
  int ReadPos(int id){return check(id)?devices[id].pos:-1;}
  int ReadSpeed(int id){++speedReads;return check(id)?devices[id].speed:-1;}
  int EnableTorque(int id,int value){if(check(id))devices[id].enabled=value;return writeByte(id,40,value);}
  int WritePosEx(int id,int pos,int,int,int){return CalibrationOfs(id,pos);}
  int WriteSpe(int id,int speed,int,int){if(check(id))devices[id].speed=speed;return writeByte(id,46,speed);}
};
