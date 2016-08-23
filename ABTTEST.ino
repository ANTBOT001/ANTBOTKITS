#include <ABTKITS.h>
#include "SoftwareSerial.h"

ABTKITS abtKits;
int cnt=0;
void setup() {
  // put your setup code here, to run once:
abtKits.ABTINIT();//初始化
delay(200);
}

void loop() {
  // put your main code here, to run repeatedly:
  //abtKits.ABTSimple();return;
  int i=0;
  i = abtKits.ABTGetBleCmd();
  delay(30);

  if(i>5)//接收字节数大于6时执行处理命令函数
  {
    abtKits.ABTHandleBleCmd();delay(30);
   }

}
