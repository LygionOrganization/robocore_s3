#include <lygion_devs.h>
TTLSDClass ttlsd;

byte ID[2];
u16 Position[2];
u16 Speed[2];
byte ACC[2];
u16 Torque[2];

void setup()
{
  Serial1.begin(1000000, SERIAL_8N1, 18, 17);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  ttlsd.pSerial = &Serial1;
  delay(1000);
  ID[0] = 1;//ID1
  ID[1] = 2;//ID2
  Speed[0] = 600;
  Speed[1] = 600;
  ACC[0] = 0;
  ACC[1] = 0;
  Torque[0] = 150;
  Torque[1] = 150;
}

void loop()
{
  Position[0] = 3200;
  Position[1] = 3200;
  ttlsd.SyncWritePosEx(ID, 2, Position, Speed, ACC, Torque);
  delay(2000);

  Position[0] = 0;
  Position[1] = 0;
  ttlsd.SyncWritePosEx(ID, 2, Position, Speed, ACC, Torque);
  delay(2000);
}
