#include <lygion_devs.h>

TTLSDClass ttlsd;

void setup()
{
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 18, 17);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  ttlsd.pSerial = &Serial1;
  delay(1000);
  ttlsd.CalibrationOfs(1, 1024);
  delay(10);
}

void loop()
{
  int pos = ttlsd.ReadPos(1);
  if(!ttlsd.getLastError()){
    Serial.print("pos:");
    Serial.println(pos);
  }
  delay(1000);
}
