#include <lygion_devs.h>

NodeClass node;

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);//esp32-s3
  //Serial1.begin(1000000);//mega2560
  node.pSerial = &Serial1;
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  node.pwmCtl(0, 1, 0);//node_id=0 pwm_ch=1 pwm_val = 0
  node.pwmCtl(0, 2, 0);//node_id=0 pwm_ch=2 pwm_val = 0
  delay(500);
  node.pwmCtl(0, 1, 500);//node_id=0 pwm_ch=1 pwm_val = 500
  node.pwmCtl(0, 2, 500);//node_id=0 pwm_ch=2 pwm_val = 500
  delay(500);
  node.pwmCtl(0, 1, 1000);//node_id=0 pwm_ch=1 pwm_val = 1000
  node.pwmCtl(0, 2, 1000);//node_id=0 pwm_ch=2 pwm_val = 1000
  delay(500);
}
