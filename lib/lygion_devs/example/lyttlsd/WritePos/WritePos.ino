#include <lygion_devs.h>

TTLSDClass ttlsd;

void setup()
{
  Serial1.begin(1000000, SERIAL_8N1, 18, 17);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  ttlsd.pSerial = &Serial1;
  delay(1000);
}

void loop()
{
  ttlsd.WritePosEx(1, 3200, 600, 0, 150);
  delay(2000);

  ttlsd.WritePosEx(1, 0, 600, 0, 150);
  delay(2000);
}
