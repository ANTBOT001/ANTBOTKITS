/*
 * ABTKITS.cpp
 * Author: ANTBOT
 * Created: 2016-05-21
 */
#include <Arduino.h> 
#include <string.h>
#include "SoftwareSerial.h"
#include "ABTKITS.h"



//#define ABT_32U4
SoftwareSerial softSerial(8, 10);//RXD,TXD 软串口

ABTKITS::ABTKITS(){
}

void ABTKITS::ABTINIT()
{
#ifdef ABT_32U4
	
	Serial.begin(9600);	//用于和PC串口通讯
	delay(200);
	Serial1.begin(9600);//用于和蓝牙模块通讯
	Serial.println("okey,ready");
#else//328P
	Serial.begin(57600);	//用于和蓝牙模块通讯
	softSerial.begin(57600);	//用于和PC串口通讯	
	//Serial.begin(9600);	//用于和蓝牙模块通讯
	//softSerial.begin(9600);	//用于和PC串口通讯	
	delay(200);
	softSerial.println("okey,ready");
#endif

  sprintf( HEADFRAME,"ABT");
  
  frameCnt=0;
  rbytes = 0;
  speedL= speedR=80; 
}
int ABTKITS::ABTGetBleCmd()
{
	//	int i=0;
#ifdef ABT_32U4
	while(Serial1.available()>0)
  {
    readcmd[rbytes++] = (char)(Serial1.read());//保存接收字符 
    delay(1);
    if(rbytes==64)
    break;
  }
#else//328P
	while(Serial.available()>0)
  {
    readcmd[rbytes++] = (char)(Serial.read());//保存接收字符 
    delay(1);
    if(rbytes==64)
    break;
  }
#endif
	return rbytes;
}
void ABTKITS::ABTSendCMD(char *p)
{
#ifdef ABT_32U4
	Serial1.println(p);//用于和蓝牙模块通讯	
#else//328P
	Serial.println(p);	//用于和蓝牙模块通讯
#endif
}
void ABTKITS::ABTSprint(char *p)
{
#ifdef ABT_32U4
	Serial.println(p);//32u4调试	
#else//328P
	softSerial.println(p);	//328p调试
#endif
}
void ABTKITS::ABTHandleBleCmd()
{
	int i,j;
  i=0;
  char cmdbuff[64];
  j=0;
  for(i=0;i<64;i++)
  {
    if(readcmd[i]!=' ')//忽略空格
    cmdbuff[j++]=readcmd[i];
    if(readcmd[i]=='#')//帧结束标志
    {
      frameCnt++;
      //Serial.println(cmdbuff);
      //if(frameCnt>6)//忽略前面几条命令
      ABTExpCmd(cmdbuff);
      j=0;
      }    
    }
    memset(readcmd,0,64);//清空接收缓存
		rbytes=0;
#ifdef ABT_32U4
        Serial1.flush();
#else
				Serial.flush();
#endif
}
void ABTKITS::ABTExpCmd(char *p)
{
	int i,j;
  char num[3];
  char cmd[32];
  char ctype,cRW;
  char headbuf[4];
  int param[2];//param[0]:管脚号/传感器模块编号 param[1]:值
  for(i=0;i<3;i++)//判断帧头
  {
    if(*p!=HEADFRAME[i])
    return;
    p++;
    }
   ctype= *p++;//命令类型：模拟量/数字量/传感器模块
   cRW  = *p++;//读写操作
  for(j=0;j<2;j++)
  {
    for(i=0;i<3;i++)
    {
      if(*p==','||*p=='#')
      break;      
      num[i]=*p;p++;
    }
    p++;
    param[j]=atoi(num);            
    memset(num,0,3);
  } 
  
  if(ctype=='A')//模拟量读写(PWM)
  {   
       if(param[0]<2)//不控制0和1管脚
  			return;  
        if(cRW=='W')
        {
          sprintf( cmd,"analogWrite(%d,%d);",param[0],param[1]);
          pinMode(param[0],OUTPUT);
          analogWrite(param[0],param[1]);
          }else if(cRW=='R')
          {
            sprintf( cmd,"analogRead(%d,%d);",param[0],param[1]);
            pinMode(param[0],INPUT);
            sprintf(sendcmd,"ABTAR%d,%d#",param[0],analogRead(param[0]));
            ABTSendCMD(sendcmd);
            }
        
        }else if(ctype=='D') //数字量读写
        {
        	if(param[0]<2)//不控制0和1管脚
  				return;  
            if(cRW=='W')
            {
              sprintf( cmd,"digitalWrite(%d,%d);",param[0],param[1]);
              pinMode(param[0],OUTPUT);
              digitalWrite(param[0],param[1]);
              }else if(cRW=='R')
              {
                sprintf( cmd,"digitalRead(%d,%d);",param[0],param[1]);
                pinMode(param[0],INPUT);
                sprintf(sendcmd,"ABTDR%d,%d#",param[0],digitalRead(param[0]));
            		ABTSendCMD(sendcmd); //              
                }
          }else //if(ctype=='S')//传感器操作  
          {
            //SnsrModel(cRW,param[0],param[1]);
            sprintf( cmd,"SensorOperate(%d,%d);",param[0],param[1]);
            ABTSensorFunc(cRW,param[0],param[1]);
            }
#ifdef ABT_32U4
        Serial.println(cmd);delay(10);
#else
				//softSerial.println(cmd);delay(10);//如果控制小车使用了10号管脚，就不要再使用该语句
#endif
}
void ABTKITS::ABTSimple()//串口透传测试，需连接电脑并打开串口监视器或串口助手
{
#ifdef ABT_32U4
    while(Serial1.available()) {    
        delay(1);Serial.write(Serial1.read());
    }
    while(Serial.available()) {
        delay(1);Serial1.write(Serial.read());
    }
#else
		while(Serial.available()) {    
        delay(1);softSerial.write(Serial.read());
    }
    while(softSerial.available()) {
        delay(1);Serial.write(softSerial.read());
    }
#endif
}
void ABTKITS::ABTPrint()
{
#ifdef ABT_32U4	
	Serial.println("Hello ABT!");
#else
  softSerial.println("Hello ABT!");
#endif	
	}
 

void ABTKITS::ABTSensorFunc(char cRW,int sID,int sVal)//传感器模块处理函数
{
	curInfo.cRW = cRW;
	curInfo.sID = sID;
	curInfo.sVal= sVal;
}
