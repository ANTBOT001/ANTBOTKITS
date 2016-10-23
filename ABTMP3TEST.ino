#include <string.h>
#include <ABTKITS.h>
#include "SoftwareSerial.h"
//////////////////////////////////////////
//TF卡中拷贝3首MP3，分别是001.mp3 002.mp3/
//003.mp3                                /
//////////////////////////////////////////
ABTKITS abtKits;
SoftwareSerial myPort(10, 8); // 软件模拟串口10为RX, 8为TX
int cnt=0;

void setup() {
  // put your setup code here, to run once:
  unsigned char cmd[6];
  abtKits.ABTINIT();
  delay(200);
  myPort.begin(9600);
  delay(200);
  cmd[0]=0x7E;cmd[1]=0x03;cmd[2]=0x35;cmd[3]=0x01;cmd[4]=0xEF;  //切换到tf卡播放模式  
  myPort.write(cmd,5); delay(200); 
  mp3Mode(0);//循环模式设置为ALL
  setVolum(10);//设置音量
}

void loop() {
  // put your main code here, to run repeatedly:
  int i=0;
  i = abtKits.ABTGetBleCmd();
  delay(20);

  if(i>6)//接收字节数大于6执行处理命令函数
  {
    Serial.println("ok cmd");
    abtKits.ABTHandleBleCmd();delay(20);
    if(abtKits.curInfo.cRW=='W')
    {
      abtKits.curInfo.cRW = 'N';//避免下一次重复进入
      if(abtKits.curInfo.sID==3)
      {
        mp3Play(abtKits.curInfo.sVal+1);
        }
    }
    
    }
  /*int i;
  setVoice(cnt+1,18);//播放第cnt+1首歌曲
  cnt++;
  if(cnt==3)
  cnt=0;
  for(i=0;i<30;i++)
  {
    delay(1000);
    }*/
}
void playMp3(unsigned char mp3Index)
{
  unsigned char cmd[6];   
  cmd[0]=0x7E;cmd[1]=0x04;cmd[2]=0x41;cmd[3]=0x0;cmd[4]=mp3Index;cmd[5]=0xEF;   
  myPort.write(cmd,5);  delay(100);
  Serial.println("play music !"); 
  }
void setVolum(unsigned char volunm)//0-30
{
  unsigned char cmd[6];  
  cmd[0]=0x7E;cmd[1]=0x03;cmd[2]=0x31;cmd[3]=volunm;cmd[4]=0xEF;     
  myPort.write(cmd,5);  delay(100);
  Serial.println("set volunm !"); 
  }

  void mp3Play(unsigned char index)//1:播放 2：暂停 3：下一曲 4：上一曲 5：音量加 6音量减
{
  unsigned char cmd[6];  
  
  cmd[0]=0x7E;cmd[1]=0x02;cmd[2]=index;cmd[3]=0xEF;     
  myPort.write(cmd,4);  delay(50);
  Serial.println(index); 
  }  
void mp3Mode(unsigned char mode)//循环播放模式0-4(ALL\FOLDER\ONE\RANDOM)
{
  unsigned char cmd[6];  
  cmd[0]=0x7E;cmd[1]=0x03;cmd[2]=0x33;cmd[3]=mode;cmd[4]=0xEF;     
  myPort.write(cmd,5);  delay(100);
  Serial.println("set loop mode !"); 
  }  
void mp3EQ(unsigned char EQ)//播放音效0-5(NO\POP\ROCK\JAZZ\CLASSIC\BASS)
{
  unsigned char cmd[6];  
  cmd[0]=0x7E;cmd[1]=0x03;cmd[2]=0x32;cmd[3]=EQ;cmd[4]=0xEF;     
  myPort.write(cmd,5);  delay(100);
  Serial.println("set EQ !"); 
  }  
void setVoice(unsigned char mp3Index,unsigned char volunm)//设置声音及音量
{
  unsigned char cmd[6]; 
  int i;  
  cmd[0]=0x7E;cmd[1]=0x03;cmd[2]=0x31;cmd[3]=volunm;cmd[4]=0xEF;    
  myPort.write(cmd,5);  delay(100);
  Serial.println("set volunm !");    
 
  cmd[0]=0x7E;cmd[1]=0x04;cmd[2]=0x41;cmd[3]=0x0;cmd[4]=mp3Index;cmd[5]=0xEF;
  myPort.write(cmd,6); delay(100);
  Serial.println("play sound !");
    
  }
