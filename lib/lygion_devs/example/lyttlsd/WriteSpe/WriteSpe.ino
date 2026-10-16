#include <lygion_devs.h>

TTLSDClass ttlsd;

void setup()
{
  Serial1.begin(1000000, SERIAL_8N1, 18, 17);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  ttlsd.pSerial = &Serial1;
  delay(1000);
  ttlsd.SetMode(1, 1);//ID1切换至电机恒速模式
}

void loop()
{
  ttlsd.WriteSpe(1, 600, 0, 150);
  delay(5000);
  
  ttlsd.WriteSpe(1, 0, 0, 150);
  delay(2000);
  
  ttlsd.WriteSpe(1, -600, 0, 150);
  delay(5000);
  
  ttlsd.WriteSpe(1, 0, 0, 150);
  delay(2000);
}
