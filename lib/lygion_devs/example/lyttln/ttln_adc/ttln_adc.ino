#include <lygion_devs.h>

NodeClass node;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  node.pSerial = &Serial1;
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  float pow_vol = node.adcPowVol(0);//node_id=0
  if(!node.getLastError()){
    Serial.print("pow_vol:");
    Serial.println(pow_vol);//pow_vol
  }else{
    Serial.println("adc read err");
  }
  delay(100);
}
