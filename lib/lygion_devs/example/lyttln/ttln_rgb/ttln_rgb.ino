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
  node.ledSingleCtrl(0, 1, 3, 0, 0);//node_id=0, rgb_ch=1, rgb=3:0:0
  node.ledSingleCtrl(0, 2, 0, 0, 3);//node_id=0, rgb_ch=2, rgb=3:0:0
  node.ledFlush(0, 2);///node_id=0, led_num=2, flush rgb
  delay(1000);

  node.ledAllCtrl(0, 2, 0, 0, 0);//node_id=0, rgb_ch=all, rgb=0:0:0
  delay(1000);

  node.ledSingleCtrl(0, 1, 0, 7, 0);//node_id=0, rgb_ch=1, rgb=0:7:0
  node.ledSingleCtrl(0, 2, 0, 7, 0);//node_id=0, rgb_ch=2, rgb=0:7:0
  node.ledFlush(0, 2);//node_id=0, led_num=2, flush rgb
  delay(1000);

  node.ledAllCtrl(0, 2, 0, 0, 0);//node_id=0, rgb_ch=all, rgb=0:0:0
  delay(1000);
  
  node.ledSingleCtrl(0, 1, 0, 0, 3);//node_id=0, rgb_ch=1, rgb=0:0:3
  node.ledSingleCtrl(0, 2, 3, 0, 0);//node_id=0, rgb_ch=2, rgb=0:0:3
  node.ledFlush(0, 2);//node_id=0, led_num=2, flush rgb
  delay(1000);

  node.ledAllCtrl(0, 2, 0, 0, 0);//node_id=0, rgb_ch=all, rgb=0:0:0
  delay(1000);
}
