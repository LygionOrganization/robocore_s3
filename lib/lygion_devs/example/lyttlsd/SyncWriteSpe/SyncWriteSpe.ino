#include <lygion_devs.h>
TTLSDClass ttlsd;

byte ID[2];
s16 Speed[2];
byte ACC[2];
u16 Torque[2];

void setup()
{
  Serial1.begin(1000000, SERIAL_8N1, 18, 17);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  ttlsd.pSerial = &Serial1;
  delay(1000);
  ttlsd.SetMode(1, 1);//ID1切换至恒速模式
  ttlsd.SetMode(2, 1);//ID2切换至恒速模式
  ID[0] = 1;//ID1
  ID[1] = 2;//ID2
  ACC[0] = 0;
  ACC[1] = 0;
  Torque[0] = 150;
  Torque[1] = 150;
}

void loop()
{
  Speed[0] = 600;
  Speed[1] = 600;
  ttlsd.SyncWriteSpe(ID, 2, Speed, ACC, Torque);
  delay(5000);

  Speed[0] = 0;
  Speed[1] = 0;
  ttlsd.SyncWriteSpe(ID, 2, Speed, ACC, Torque);
  delay(2000);
  
  Speed[0] = -600;
  Speed[1] = -600;
  ttlsd.SyncWriteSpe(ID, 2, Speed, ACC, Torque);
  delay(5000);

  Speed[0] = 0;
  Speed[1] = 0;
  ttlsd.SyncWriteSpe(ID, 2, Speed, ACC, Torque);
  delay(2000);
}
