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
  //node_id=0, read subs buf
  if(node.sbusFlush(0)){
    Serial.print("status:");
    Serial.println(node.sbusStatus());//subs status
    for(int ch=1; ch<=node.subsGetNum(); ch++){
      Serial.print("ch");
      Serial.print(ch);
      Serial.print(":");
      Serial.println(node.sbusGetch(ch));//subs ch:1~16
    }
  }else{
    Serial.println("subs buf err");
  }
  delay(100);
}
