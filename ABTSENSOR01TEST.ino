#include <ABTKITS.h>

#include "SoftwareSerial.h"

#define NOTE_CS5 554 //3
#define NOTE_D5  587 //4
#define NOTE_E5  659 //5

#define K1_PIN   2
#define K2_PIN   4
#define K3_PIN   3
#define K4_PIN   5

ABTKITS abtKits;
extern SoftwareSerial softSerial;
int cnt=0;
int spkType=0;//报警类型
void setup() {
  // put your setup code here, to run once:
abtKits.ABTINIT();
delay(1000);
pinMode(K1_PIN,INPUT);//K1
pinMode(K2_PIN,INPUT);//K2
pinMode(K3_PIN,INPUT);//K3
pinMode(K4_PIN,INPUT);//K4
pinMode(7,OUTPUT);//Buzzer ctrl
//softSerial.begin(2400);//更改软串口波特率，用于接收PM2.5数据
delay(500);

}

void loop() {
  // put your main code here, to run repeatedly:
  //abtKits.ABTSimple();return;
  //int i=0;
  //i = abtKits.ABTGetBleCmd();
  //delay(20);
  char sendcmd[64];
  int pmData = GetPM25Data();
  if(pmData>0)
  {
    if(pmData>50)
    WarnSound(spkType);//PM2.5报警设置
    sprintf(sendcmd,"ABTSR02,%d#",pmData);//PM2.5
    abtKits.ABTSendCMD(sendcmd);delay(100);
    sprintf(sendcmd,"ABTSR00,%d#",cnt++);//温度
    abtKits.ABTSendCMD(sendcmd);delay(100);
    sprintf(sendcmd,"ABTSR01,%d#",cnt+30);//湿度
    abtKits.ABTSendCMD(sendcmd);delay(100);
    }
    if(digitalRead(K1_PIN)==0)
    {
      spkType = 1;
      WarnSound(spkType);//设置报警音
      }
      if(digitalRead(K2_PIN)==0)
    {
      spkType = 2;
      WarnSound(spkType);//设置报警音
      }
      if(digitalRead(K3_PIN)==0)
    {
      spkType = 3;
      WarnSound(spkType);//设置报警音
      }
      if(digitalRead(K4_PIN)==0)
    {
      spkType = 4;
      WarnSound(spkType);//设置报警音
      }
    delay(500);
}

int GetPM25Data()//读取PM2.5传感器
{
  int cnt=0;
  int data=0;
  int revbuf[7];
  int dsize=0;
  while (softSerial.available()>0){
   data = softSerial.read();     
   dsize++;
   if(dsize>100)//超时退出
   {
    return -1;
    }
   if(cnt==0)
   {
    if(data==0xAA)//帧头
    revbuf[0] = data;
    }
    if(revbuf[0]==0xAA)
    {
      revbuf[cnt++]=data;
      }
   if(cnt==7)//帧计数
   {
    cnt=0;
    break;
    }  
   delay(10);
  }
  int sum=revbuf[1]+ revbuf[2]+ revbuf[3] + revbuf[4];
  if(revbuf[5]==sum && revbuf[6]==0xff )//校验字和结束字
  {
    float vo=((revbuf[1]<<8)+revbuf[2])/1024.0*5.00;//计算PM2.5值
    softSerial.flush();
    revbuf[0]=0;    
    return vo*800;   //返回读数
    
    }
    
    return -1;//校验字或结束字错误
  }
void WarnSound(int Type)
{
  int tonePin = 7;
  if(Type>4)
  return;
  int melody[] = {
NOTE_CS5,//
NOTE_D5,//
NOTE_E5,//
NOTE_E5,//
0,
};

int noteDurations[] = {
  4,4,4,4,
  4,  
};
for (int thisNote = 0; thisNote < Type; thisNote++) {

    // to calculate the note duration, take one second 
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(tonePin, melody[thisNote],noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(tonePin);
  }
  
  }
