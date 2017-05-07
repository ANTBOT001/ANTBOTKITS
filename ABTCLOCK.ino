// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include <ABTKITS.h>
#include <SoftwareSerial.h>
#include <dht11.h>//包含头文件
dht11 DHT11;      //声明对象实例
#define DHT11PIN  6//指定D6管脚为单总线数据接口
#define ICON_DAY  3
#define ICON_NIGHT  4
#define PIC_DAY  4
#define PIC_NIGHT  5
#define CH_NIGHT  4
#define CH_DAY  3

#define NOTE_D1  294 //音符1
#define NOTE_D2  330 //音符2
#define NOTE_D3  350 //音符3

ABTKITS abtKits;//ABT实例
RTC_DS1307 RTC;
SoftwareSerial mySerial(8,10);//设置软串口，8为RX，10为TX
unsigned char add0 = 2;
unsigned char add1 = 3;
unsigned char oldHour = 0;
unsigned char dayFlag = 0;
unsigned char picBak = PIC_DAY;
unsigned char chClr = CH_DAY;
unsigned char iconIndex = ICON_DAY;

unsigned char clkFlag = 0;//闹铃标志
unsigned char tmrFlag = 0;//计时标志
unsigned char clk_h = 6; //闹铃小时
unsigned char clk_m = 30; //闹铃分钟
unsigned char tmr_m = 0;  //定时分钟
unsigned char tmr_s = 0;  //定时分钟
unsigned char tmr_hc = 0;  //定时分钟
unsigned char tmr_mc = 0;  //定时分钟
unsigned char tmr_sc = 0;  //定时分钟
unsigned char ringCnt=0;

//unsigned char icon_x[24]={204,186,168,150,132,114,96,78,60,42,24,6};
//图标出现在24小时的位置上：0, 1,2,  3, 4, 5, 6,  7,  8,  9, 10, 11, 12,13,14,15,16,17,18, 19, 20, 21, 22,23
unsigned char icon_x[24]={114,96,78,60,42,24, 6,204,186,168,150,132,114,96,78,60,42,24, 6,204,186,168,150,132};
unsigned char icon_y[24]={ 0, 0,  0,20,20,20,80, 80, 20, 20,  0,  0,  0, 0, 0, 0, 0,20,80, 80, 20, 20,  0, 0};

void setup () {
  pinMode(add0,OUTPUT);
  pinMode(add1,OUTPUT); 
  pinMode(4,INPUT);
  pinMode(5,INPUT);
  //////////////////////////
  clk_h = ReadCfg(0);
  if(clk_h>23)
  clk_h = 0;
  clk_m = ReadCfg(1);
  if(clk_m>59)
  clk_m = 0;
  /////////////////////////
 //   Serial.begin(57600);
  abtKits.ABTINIT();delay(100);
    Wire.begin();
    RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  //选中COM3用于控制液晶屏
  digitalWrite(add0,1);
  digitalWrite(add1,1);
  delay(10);  
  mySerial.begin(115200);
  mySerial.println("CLS(0);");
  delay(200); 
  mySerial.println("BPIC(4,0,0,4);"); //设置背景图片：4.jpg
  delay(200);
  mySerial.println("ICON(40,110,6,5,1,0);"); //设置温度图标
  delay(200);
  mySerial.println("ICON(130,110,6,5,1,1);"); //设置湿度度图标
  delay(200);
  mySerial.println("ICON(110,5,6,5,1,3);"); //设置太阳图标
  delay(200);
}

void loop () {
   char cmd[64];
   int newHour,newMin,newSec;
   int chk;
  ///////////////////////////////////////////
   int rbytes = abtKits.ABTGetBleCmd();
   if(rbytes>6)
   {
     abtKits.ABTHandleBleCmd();
     int cmdt = abtKits.senCnt;
     int i;
     int clkchange = 0;
     for(i=0;i<cmdt;i++)
     {
        if(abtKits.senInfo[i].sID==7)//闹钟小时
        {
          if(clk_h != abtKits.senInfo[i].sVal)
          {
            clk_h = abtKits.senInfo[i].sVal;
            clkchange = 1;
            WriteCfg(0,clk_h);
            }
          
          }else if(abtKits.senInfo[i].sID==8)//闹钟分钟
          {
            if(clk_m != abtKits.senInfo[i].sVal)
            {
              clk_m = abtKits.senInfo[i].sVal;
              clkchange = 1;
              WriteCfg(0,clk_m);
              }
            
            }else if(abtKits.senInfo[i].sID==9)//定时分钟
          {
            tmr_m = abtKits.senInfo[i].sVal;
            }else if(abtKits.senInfo[i].sID==10)//定时秒
          {
            tmr_s = abtKits.senInfo[i].sVal;
            }
      }
      if(clkchange)
      {
        sprintf(cmd,"PS16(%d,40,160,'^O^%02d:%02d',1);",picBak,clk_h,clk_m);
        mySerial.println(cmd);delay(50);
        }
      return;
     
    }  
  ////////////////////////////////////////// 
   DateTime nowto = RTC.now();
   // DateTime nowt = RTC.now();
   DateTime nowt (nowto.unixtime() +120);
   newHour  = nowt.hour();
   newMin   = nowt.minute();
   newSec   = nowt.second();
   int tmrcnt = tmr_m*60+tmr_s;
  if(tmrcnt>0)
  {
    tmr_m = 0;
    tmr_s = 0;
    DateTime dstTime(nowto.unixtime() +120+tmrcnt);
    tmr_hc = dstTime.hour();
    tmr_mc = dstTime.minute();
    tmr_sc = dstTime.second();
    sprintf(cmd,"PS16(%d,110,160,'->%02d:%02d:%02d',1);",picBak,tmr_hc,tmr_mc,tmr_sc);
    mySerial.println(cmd);delay(50);
    }
   
   if(tmrFlag==0)
    {
      if(tmr_hc==newHour&&tmr_mc==newMin&&newSec==newSec)
      {
          tmrFlag=1;
          tmr_hc = 0;tmr_mc=0;tmr_sc=0;
          sprintf(cmd,"PS16(%d,110,160,'-:-:-     ',1);",picBak);
          mySerial.println(cmd);delay(50);
      }
     }else{
       WarnSound(); 
        ringCnt++;
        chk = digitalRead(5);
        if(ringCnt==10||chk==0)
        {
          tmrFlag=0;
          ringCnt=0;
          }
         delay(800);
         return;    
      }
   if(clkFlag==0)
   {
      if(newHour==clk_h&&newMin==clk_m&&newSec>1&&newSec<5)
     {
        clkFlag=1;       
      }
    }else{
        WarnSound(); 
        ringCnt++;
        chk = digitalRead(5);
        if(ringCnt==10||chk==0)
        {
          clkFlag=0;
          ringCnt=0;
          }
        
        delay(800);
        return;     
      }
    
    
   
    if(newHour!=oldHour)//设置太阳或月亮图标
    {
      WarnSound();
      oldHour = newHour;
      if(newHour>7&&newHour<19)//白天
      {
        dayFlag =1;
        picBak = PIC_DAY;
        chClr = CH_DAY;
        iconIndex = ICON_DAY;
        mySerial.println("BPIC(4,0,0,4);"); //设置背景图片：4.jpg
        delay(50);
        
        //mySerial.println("ICON(110,5,6,5,1,3);"); //设置太阳图标
        sprintf(cmd,"ICON(%d,%d,6,5,1,3);",icon_x[newHour],icon_y[newHour]);
        mySerial.println(cmd);
        delay(50);
        //闹铃
        sprintf(cmd,"PS16(%d,40,160,'^O^%02d:%02d',1);",picBak,clk_h,clk_m);
        mySerial.println(cmd);delay(50);
        }else{//夜晚
          dayFlag =0;
          picBak = PIC_NIGHT;
          chClr = CH_NIGHT;
          iconIndex = ICON_NIGHT;
          mySerial.println("BPIC(5,0,0,5);"); //设置夜晚背景图片：4.jpg
          delay(50);
          //mySerial.println("ICON(110,5,6,5,1,4);"); //设置月亮图标
          sprintf(cmd,"ICON(%d,%d,6,5,1,4);",icon_x[newHour],icon_y[newHour]);
          mySerial.println(cmd); //设置月亮图标
          delay(50);
          }
          mySerial.println("ICON(40,110,6,5,1,0);"); //设置温度图标
          delay(50);
          mySerial.println("ICON(130,110,6,5,1,1);"); //设置湿度度图标
          delay(50);
          //闹铃
          sprintf(cmd,"PS16(%d,40,160,'^O^%02d:%02d',1);",picBak,clk_h,clk_m);
          mySerial.println(cmd);delay(50);
      }
      sprintf(cmd,"PS16(%d,70,35,'%02d-%02d-%02d',%d);",picBak,nowt.year(),nowt.month(),nowt.day(),chClr);
      mySerial.println(cmd);delay(50);
      sprintf(cmd,"PS48(%d,40,55,'%02d:%02d:%02d',%d);",picBak,nowt.hour(),nowt.minute(),nowt.second(),chClr);
      mySerial.println(cmd);delay(50);
      delay(800);
    chk = DHT11.read(DHT11PIN);//读取DHT11数据，并获得错误码
    if(chk==DHTLIB_OK)
    {
        
        sprintf(cmd,"PS32(%d,75,110,'%02d',%02d);PS32(%d,165,110,'%02d',%02d);",picBak,DHT11.temperature-5,chClr,picBak,DHT11.humidity,chClr);        
        mySerial.println(cmd); 
      }
     delay(100);
    /*
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
    
    // calculate a date which is 7 days and 30 seconds into the future
    DateTime future (now.unixtime() + 7 * 86400L + 30);
    
    Serial.print(" now + 7d + 30s: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();
    
    Serial.println();
    delay(3000);*/
}
void WarnSound()
{
  int tonePin = 7;  
  int melody[] = {
NOTE_D1,//
NOTE_D2,//
NOTE_D3,//
NOTE_D1,//
0,
};

int noteDurations[] = {
  4,4,4,8,
  4,  
};
for (int thisNote = 0; thisNote < 4; thisNote++) {

    // to calculate the note duration, take one second 
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(tonePin, melody[thisNote],noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.20;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(tonePin);
  }
  
  }
 unsigned char ReadCfg(unsigned char addr)
 {
      unsigned char val;
      val =  EEPROM.read(addr);      
      return val;
      
  }
  void WriteCfg(unsigned char addr,unsigned char val)
 {
      EEPROM.write(addr, val);
  }
