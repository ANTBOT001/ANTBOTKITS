#include <ABTKITS.h>

#include "SoftwareSerial.h"
#include <dht11.h>

dht11 DHT11;

#define DHT11PIN 6
#define NOTE_CS5 554 //3
#define NOTE_D5  587 //4
#define NOTE_E5  659 //5

#define K1_PIN   2
#define K2_PIN   4
#define K3_PIN   3
#define K4_PIN   5

ABTKITS abtKits;
SoftwareSerial softSerial(8, 10);//RXD,TXD 初始化波特率应改为2400
int cnt=0;
int spkType=0;//报警类型
char sendcmd[64];
int oldPm = 0;
void setup() {
  // put your setup code here, to run once:
abtKits.ABTINIT();
delay(1000);
pinMode(K1_PIN,INPUT);//K1
pinMode(K2_PIN,INPUT);//K2
pinMode(K3_PIN,INPUT);//K3
pinMode(K4_PIN,INPUT);//K4
pinMode(7,OUTPUT);//Buzzer ctrl
softSerial.begin(2400);//更改软串口波特率，用于接收PM2.5数据
delay(200);

}

void loop() {
  // put your main code here, to run repeatedly:
  
  
  float pmData = GetPM25Data();
  delay(100);
  if(pmData>0)
  {
     sprintf(sendcmd,"ABTSR02,%d#",(int)pmData);//PM2.5
     abtKits.ABTSendCMD(sendcmd);delay(100);
     }
  int chk = DHT11.read(DHT11PIN);
  delay(200);
  if(chk==DHTLIB_OK)
  {
    sprintf(sendcmd,"ABTSR00,%d#",DHT11.temperature);//温度
    abtKits.ABTSendCMD(sendcmd);delay(100);
    sprintf(sendcmd,"ABTSR01,%d#",DHT11.humidity);//湿度
    abtKits.ABTSendCMD(sendcmd);delay(100);
    }
    
}

float GetPM25Data()//读取PM2.5传感器,波特率：2400； 校验位：无； 停止位：1 位； 数据位：8；数据包长度为7字节
{
  int cnt,pmval,readcmd[7];
  unsigned char gdata,eFlag,rbytes=0;
  int pm25;
  eFlag=0;
  cnt=0;
  while(softSerial.available()>0)
  {
    gdata = softSerial.read();//保存接收字符 
    if(gdata==0xAA&&eFlag==0)
     {
        eFlag=1;        
    }
    if(eFlag==1)
    {
        readcmd[rbytes++]=gdata;
    }    
    delay(2);
    cnt++;
    if(cnt>400)
    return 0;
    if(rbytes==7)//完整帧
    {
      break;
      }   
    }
    if(rbytes==0)
     return 0;
    //if(readcmd[6]!=0xFF)
    // return 0;
  pmval = readcmd[1];
  pmval<<=8;
  pmval+=readcmd[2];
  pm25 = pmval*5.0/1024.0;//计算PM2.5值
  pm25*=800.0;
  if(pm25>999)
  pm25=0;
  return pm25;
}
void WarnSound(int Type)
{
  return;
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
  double Fahrenheit(double celsius) 
{
        return 1.8 * celsius + 32;
}    //摄氏温度度转化为华氏温度
