/*
回读所有反馈参数:位置、速度、负载、电压、温度、移动状态、电流；
FeedBack函数回读舵机参数于缓冲区，Readxxx(-1)函数返回缓冲区中相应的舵机状态；
函数Readxxx(ID)，ID=-1返回FeedBack缓冲区参数；ID>=0，通过读指令直接返回指定ID舵机状态,
无需调用FeedBack函数。
*/

#include <lygion_devs.h>

TTLSDClass ttlsd;

void setup()
{
  //Serial1.begin(1000000);//mega2560
  Serial1.begin(1000000, SERIAL_8N1, 18, 17);//esp32-s32
  Serial.begin(115200);
  ttlsd.pSerial = &Serial1;
  delay(1000);
}

void loop()
{
  int Pos;
  int Speed;
  int Load;
  int Voltage;
  int Temper;
  int Move;
  int Current;
  ttlsd.FeedBack(1);
  if(!ttlsd.getLastError()){
    Pos = ttlsd.ReadPos(-1);
    Speed = ttlsd.ReadSpeed(-1);
    Load = ttlsd.ReadLoad(-1);
    Voltage = ttlsd.ReadVoltage(-1);
    Temper = ttlsd.ReadTemper(-1);
    Move = ttlsd.ReadMove(-1);
    Current = ttlsd.ReadCurrent(-1);
    Serial.print("Position:");
    Serial.println(Pos);
    Serial.print("Speed:");
    Serial.println(Speed);
    Serial.print("Load:");
    Serial.println(Load);
    Serial.print("Voltage:");
    Serial.println(Voltage);
    Serial.print("Temper:");
    Serial.println(Temper);
    Serial.print("Move:");
    Serial.println(Move);
    Serial.print("Current:");
    Serial.println(Current);
    delay(10);
  }else{
    Serial.println("FeedBack err");
    delay(500);
  }
  
  Pos = ttlsd.ReadPos(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo position:");
    Serial.println(Pos, DEC);
    delay(10);
  }else{
    Serial.println("read position err");
    delay(500);
  }
  
  Voltage = ttlsd.ReadVoltage(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo Voltage:");
    Serial.println(Voltage, DEC);
    delay(10);
  }else{
    Serial.println("read Voltage err");
    delay(500);
  }
  
  Temper = ttlsd.ReadTemper(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo temperature:");
    Serial.println(Temper, DEC);
    delay(10);
  }else{
    Serial.println("read temperature err");
    delay(500);    
  }

  Speed = ttlsd.ReadSpeed(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo Speed:");
    Serial.println(Speed, DEC);
    delay(10);
  }else{
    Serial.println("read Speed err");
    delay(500);    
  }
  
  Load = ttlsd.ReadLoad(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo Load:");
    Serial.println(Load, DEC);
    delay(10);
  }else{
    Serial.println("read Load err");
    delay(500);    
  }
  
  Current = ttlsd.ReadCurrent(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo Current:");
    Serial.println(Current, DEC);
    delay(10);
  }else{
    Serial.println("read Current err");
    delay(500);    
  }

  Move = ttlsd.ReadMove(1);
  if(!ttlsd.getLastError()){
    Serial.print("Servo Move:");
    Serial.println(Move, DEC);
    delay(10);
  }else{
    Serial.println("read Move err");
    delay(500);    
  }
  Serial.println();
}
