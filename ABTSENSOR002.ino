#include <SoftwareSerial.h>
SoftwareSerial mySerial(8,10);
unsigned char add0 = 2;
unsigned char add1 = 3;
unsigned char btn0 = 4;
unsigned char btn1 = 5;

//液晶板KHM1207A定义
unsigned char DATA=11;
unsigned char W_R=12;
unsigned char CS=13;
unsigned char dataT[10]={0xeb,0x0a,0xad,0x8f,0x4e,0xc7,0xe7,0x8a,0xef,0xcf};//0-9
unsigned char dataD[15];//display_data
///////////////////////////
unsigned char pmType=0;//传感器类型选择0：益衫 1：sharp 2:CH2O(ZE08甲醛传感器)
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(add0,OUTPUT);
  pinMode(add1,OUTPUT);
  pinMode(DATA,OUTPUT);
  pinMode(W_R,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(btn0,INPUT);
  pinMode(btn1,INPUT);

  mySerial.begin(9600);
  delay(100);
  SetCom(0);
  digitalWrite(CS,1);
  digitalWrite(W_R,0);
   initial();
  delay(100);
  displayData(888.8);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned char data;  
  float val_pm25;
  if(digitalRead(btn0)==0)
  {
    pmType++;
    }
    if(pmType==3)
    pmType=0;
    if(pmType==0)
    {
      SetCom(0);      
      mySerial.begin(9600);
      delay(100);
      val_pm25 = (float)GetPM25Data_yishan();//益衫PM2.5传感器
      }else if(pmType==1){
        SetCom(1);      
      mySerial.begin(9600);
      delay(100);
      val_pm25 = GetPM25Data_CH2O();//甲醛传感器
        }else if(pmType==2){
        SetCom(2);      
      mySerial.begin(2400);
      delay(100);
      val_pm25 = GetPM25Data_sharp();//夏普PM2.5传感器
        }
        if(val_pm25>0)
        displayData(val_pm25);
        //mySerial.close();
        delay(200);
  /*while (mySerial.available()>0){
   data = mySerial.read();   
   mySerial.write(data);
   delay(2);
  }*/
}
void SetCom(int index)
{
  if(index==0)
  {
    digitalWrite(add0,0);digitalWrite(add1,0);
    
    }else if(index==1)
    {
      digitalWrite(add0,1);digitalWrite(add1,0);      
      }else if(index==2)
    {
      digitalWrite(add0,0);digitalWrite(add1,1);      
      }else if(index==3)
    {
      digitalWrite(add0,1);digitalWrite(add1,1);      
      }
 }
 void initial()
{
  digitalWrite(CS,0);
  digitalWrite(DATA,1); wave();   //command mode
  digitalWrite(DATA,0); wave();  
  digitalWrite(DATA,0);wave();

  tran_inst(0x18);    //RC 256K
  tran_inst(0x00);
  tran_inst(0x01);    //turn on system oscilator 
  tran_inst(0x03);    //turn on bias generator
  tran_inst(0x29);    //1/3 bias 4 commons//    1/2 bias 3 commons//0x04
  //tran_inst(0xe0);    //turn on lcd output
  digitalWrite(CS,1);
}


void tran_inst(unsigned char buffer)  
    {     
    unsigned char i;
    unsigned char wDATA;
    for(i=0;i<8;i++)
           {
              wDATA=((buffer&0x80)==0x80);digitalWrite(DATA,wDATA);
        wave();
              buffer<<=1;
            }
    wDATA=0;digitalWrite(DATA,wDATA);
    wave();                
     }

void tran_data(unsigned char *p)
    { 
      unsigned char i,j,wbuffer,address=0;
      unsigned char wDATA;
    
   digitalWrite(CS,0);  
   digitalWrite(DATA,1); wave();    
   digitalWrite(DATA,0); wave();      
   digitalWrite(DATA,1); wave();
    
    for(i=0;i<6;i++)
           {
              wDATA=((address&0x80)==0x80);
              digitalWrite(DATA,wDATA);
        wave();
              address<<=1;
            }
    
    for(i=0;i<15;i++)//数据位数，显示每组数据的第几个数据，对应表格中的数据
      { 
        wbuffer=*p++;            
          for(j=0;j<8;j++)
             {
                wDATA=((wbuffer&0x80)==0x80);digitalWrite(DATA,wDATA);
          wave();
                wbuffer<<=1;
              } 
      }
    digitalWrite(CS,1);            
     }
void wave()      // generate a waveform
{
  digitalWrite(W_R,0);  delay(1);
  digitalWrite(W_R,1);  delay(1);
  digitalWrite(W_R,0);  delay(1);
}
void displayData(float val)
{
  int vi = val*10;
  dataD[0] = dataT[vi/1000];//百位
  vi = vi%1000;
  dataD[1] = dataT[vi/100];//十位
  vi = vi%100;
  dataD[2] = dataT[vi/10];//个位
  
  vi = vi%10;
  dataD[3] = dataT[vi];   //小数位
  dataD[3]|=0x10;         //小数点
  tran_data(dataD);
 }
 int GetPM25Data_yishan()//读取PM2.5传感器,波特率：9600； 校验位：无； 停止位：1 位； 数据位：8；数据包长度为 32 字节
{
  int cnt,pmval,readcmd[32];
  unsigned char gdata,eFlag,rbytes=0;
  cnt=0;
  while(mySerial.available()>0)
  {
    gdata = mySerial.read();//保存接收字符 
    if(gdata==0x32&&eFlag==0)
     {
        eFlag=1;        
    }
    if(eFlag==1)
    {
        readcmd[rbytes++]=gdata;
    }
    if(rbytes==2)
    {
        if(readcmd[1]!=0x3d)
        {
            eFlag=0;
            rbytes=0;
            readcmd[0]=0;
        }
          
    }
    delay(2);
    cnt++;
    if(cnt>400)
    return 0;
    if(rbytes==32)//完整帧
    {
      break;
      }   
    }
    if(rbytes==0)
     return 0;
  pmval = readcmd[6];
  pmval<<=8;
  pmval+=readcmd[7];
  if(pmval>999)
  pmval=999;
  return pmval;
}
float GetPM25Data_sharp()//读取PM2.5传感器,波特率：2400； 校验位：无； 停止位：1 位； 数据位：8；数据包长度为7字节
{
  int cnt,pmval,readcmd[7];
  unsigned char gdata,eFlag,rbytes=0;
  float pm25;
  cnt=0;
  while(mySerial.available()>0)
  {
    gdata = mySerial.read();//保存接收字符 
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
    if(cnt>100)
    return 0;
    if(rbytes==7)//完整帧
    {
      break;
      }   
    }
    if(rbytes==0)
     return 0;
    if(readcmd[6]!=0xFF)
     return 0;
  pmval = readcmd[1];
  pmval<<=8;
  pmval+=readcmd[2];
  pm25 = pmval*5.0/1024.0;//计算PM2.5值
  pm25*=800.0;
  if(pm25>999)
  pm25=0;
  return pm25;
}
int GetPM25Data_CH2O()//读取甲醛传感器,波特率：9600； 校验位：无； 停止位：1 位； 数据位：8；数据包长度为 9 字节
{
  int cnt,pmval,readcmd[32];
  unsigned char gdata,eFlag,rbytes=0;
  cnt=0;
  while(mySerial.available()>0)
  {
    gdata = mySerial.read();//保存接收字符 
    if(gdata==0xFF&&eFlag==0)
     {
        eFlag=1;        
    }
    if(eFlag==1)
    {
        readcmd[rbytes++]=gdata;
    }
    if(rbytes==2)
    {
        if(readcmd[1]!=0x17)
        {
            eFlag=0;
            rbytes=0;
            readcmd[0]=0;
        }
          
    }
    delay(2);
    cnt++;
    if(cnt>400)
    return 0;
    if(rbytes==9)//完整帧
    {
      break;
      }   
    }
    if(rbytes==0)
     return 0;
  pmval = readcmd[4];
  pmval<<=8;
  pmval+=readcmd[5];
  if(pmval>999)
  pmval=999;
  return pmval;
}
