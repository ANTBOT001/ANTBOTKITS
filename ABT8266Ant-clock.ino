#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SH1106Wire.h" 
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#define DEBUG //是否启用debug功能
#include <EEPROM.h>
#include <DNSServer.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESPDash.h>
#include <Adafruit_NeoPixel.h>

#define PIN        0  //GPIO0接口

#define NUMPIXELS  2  //彩灯个数
#define LED_Pin D0
#define BTN_Pin D7
Adafruit_NeoPixel led(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
String my_led_status = "off";
SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL
char get_ssid[40] = {0}; //从EEPROM中读取得到的SSIDchar类型数组
char get_psk[50] = {0}; //从EEPROM中读取得到的WIFI密码
const unsigned long HTTP_TIMEOUT = 5000;               // max respone time from server
const size_t MAX_CONTENT_SIZE = 2048;                   // max size of the HTTP response
const char ssid[] = "xxxxxxxx";     //修改为自己的路由器用户名
const char password[] = "xxxxxxxx"; //修改为自己的路由器密码 
const char* host = "api.seniverse.com";
const char* APIKEY = "xxxxxxxxxxxxxx";        //心知天气网站提供的API KEY
const char* city = "beijing";
const char* language = "en";//zh-Hans 简体中文  会显示乱码
unsigned int cntFlag=0; 
unsigned char tcnt=0; 
char response[MAX_CONTENT_SIZE];
char responseLife[MAX_CONTENT_SIZE];
char endOfHeaders[] = "\r\n\r\n"; 
String weekday[8] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday",""};
String SSID_S;
String psk_s;
#define addr 200   //这个地址可以随意设置，如果用本程序则不能设置为小于50以下的值（为读取密码字符串预留空间）
#define addr1 400
#define addr2 600
#define addr3 740

#define MAX_PACKETSIZE 512
#ifdef DEBUG
#define DebugPrintln(message)    Serial.println(message)
#define DebugPrint(message)    Serial.print(message)
#else
#define DebugPrintln(message)
#define DebugPrint(message)
#endif
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com",60*60*8, 30*60*1000);
bool preTCPConnected = false;

//led控制函数，具体函数内容见下方
void turnOnLed();
void turnOffLed();
int wifiIsOk=0;
int m ; //读取字符串长度的缓冲变量
// 我们要从此网页中提取的数据的类型
struct UserData {
  char city[16];//城市名称
  char weather[32];//天气介绍（多云...）
  char temp[16];//温度  
  char udate[32];//更新时间
};
//生活指数
struct UserDataLife {
  char dressing[16];//穿衣指数
  char uv[16];//紫外线强度
  char traffic[16];//心情指数
  char dating[16];//约会指数
  char sport[16];//舒适指数
  char flu[16];//感冒指数
  };
union data  //定义一个联合体数据类型
{
  char a;
  byte b[2];
};
 
data col[40];  //两个缓存联合体用来分别用来读取 WIFI密码和名称
data col_2[50];

//两个用来分别转换字符串的数组
void change(String name1) {
 
  for (int i = 0; i < name1.length(); i++) {
    col[i].a = name1[i];
  }
}
 
void change_2(String name1) {
 
  for (int i = 0; i < name1.length(); i++) {
    col_2[i].a = name1[i];
  }
}

//设置连接热点
 
const char* AP_NAME = "Hello-ESP-WIFI";//wifi名字
//暂时存储wifi账号密码

//设置页面代码
const char* page_html = "\
<!DOCTYPE html>\r\n\
<html lang='en'>\r\n\
<head>\r\n\
  <meta charset='UTF-8'>\r\n\
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n\
  <title>Document</title>\r\n\
</head>\r\n\
<body>\r\n\
  <form name='input' action='/' method='POST'>\r\n\
        wifi名称: <br>\r\n\
        <input type='text' name='ssid'><br>\r\n\
        wifi密码:<br>\r\n\
        <input type='text' name='password'><br>\r\n\
            <br>\r\n\
            <br>\r\n\
        <input type='submit' value='保存'>\r\n\
        <a href='./m?x1=1'><input type='button'value='重启并连接'>\r\n\
    </form>\r\n\
</body>\r\n\
</html>\r\n\
";
 
const byte DNS_PORT = 53;//DNS端口号
IPAddress apIP(192, 168, 4, 1);//esp8266-AP-IP地址
DNSServer dnsServer;//创建dnsServer实例
//ESP8266WebServer server(80);//创建WebServer

AsyncWebServer server(80);
ESPDash dashboard(&server); 

Card mslider(&dashboard, SLIDER_CARD, "LED Red", "", 0, 255);
Card mslider1(&dashboard, SLIDER_CARD, "LED Green", "", 0, 255);
Card mslider2(&dashboard, SLIDER_CARD, "LED Blue", "", 0, 255);

Card button1(&dashboard, BUTTON_CARD, "My Light");
//Card button2(&dashboard, BUTTON_CARD, "On");
int lightR=10;
int lightG=10;
int lightB=10; 

int ledOn = 0;
void handleRoot(AsyncWebServerRequest *request) {//访问主页回调函数
  request->send(200, "text/html", page_html);
  if (request->arg("btn1") == "1") {
    ESP.restart();
    request->send(200, "text/html", "<meta charset='UTF-8'>重启成功");//返回保存成功页面
  }
}
 
 
void handleRootPost(AsyncWebServerRequest *request) {//Post回调函数
  Serial.println("handleRootPost");
 
 
  if (request->hasArg("ssid")) {//判断是否有账号参数
    Serial.print("got ssid:");
    strcpy(get_ssid, request->arg("ssid").c_str());//将账号参数拷贝到get_ssid中
    Serial.println(get_ssid);
 
  } else {//没有参数
    Serial.println("error, not found ssid");
   // server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");//返回错误页面
   request->send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");
    return;
  }
  //密码与账号同理
  if (request->hasArg("password")) {
    Serial.print("got password:");
    strcpy(get_psk, request->arg("password").c_str());
    Serial.println(get_psk);
  } else {
    Serial.println("error, not found password");
   // server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
   request->send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }  
  Serial.print("start to connecting...");
//delay(2000);
 request->send(200, "text/html", "ok!");
 wifiIsOk =2;
 ////////////////////////////
//  request->send(200);
  
 // connectNewWifi();
}
 
void initBasic(void) { //初始化基础
  //  Serial.begin(115200);
  WiFi.hostname("Smart-ESP8266");//设置ESP8266设备名
}
 
void initSoftAP(void) { //初始化AP模式
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (WiFi.softAP(AP_NAME)) {
    Serial.println("ESP8266 SoftAP is right");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(8, 0, "setup your wifi...");
    display.drawString(8,16, "please find ssid:");
    display.drawString(8,32, "Hello-ESP-WIFI");
    display.drawString(8,48, "andconnect to this ssid");
    display.display();
  }
}
//void callback(AsyncWebServerRequest *request){request->send(200);}
 
void initWebServer(void) { //初始化WebServer
  server.on("/", HTTP_GET, handleRoot);//设置主页回调函数  
  server.on("/", HTTP_POST, handleRootPost);//设置Post请求回调函数
  server.onNotFound(handleRoot);//设置无法响应的http请求的回调函数
  server.begin();//启动WebServer
  Serial.println("WebServer started!");
}
 
 
 
void connectNewWifi(void) {
  //  Serial.println(get_ssid);
  //    Serial.print(get_psk);
  
  WiFi.mode(WIFI_STA);//切换为STA模式
  WiFi.setAutoConnect(true);//设置自动连接
  WiFi.begin(get_ssid, get_psk);
 
  Serial.print("");
  Serial.print("Connect to wifi");
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    count++;
    if (count > 20) { //如果5秒内没有连上，就开启Web配网 可适当调整这个时间
      initSoftAP();
      initWebServer();
      initDNS();
      wifiIsOk = 0;
      break;//跳出 防止无限初始化
    }
    Serial.print(".");
  }
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) { //如果连接上 就输出IP信息 防止未连接上break后会误输出
    Serial.println("WIFI Connected!");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());//打印esp8266的IP地址
    for (int i = 0; i < 1024; i++)
      EEPROM.write(i, 0);
    EEPROM.commit();

    change(get_ssid);  //将WIFI的名称和密码转换到缓存数组当中去
    change_2(get_psk);

    Serial.println("已缓存");
    for (int h = 0 ; h < WiFi.SSID().length(); h++) //将名称一个字节一个字节缓存进EEPROM里面
    {
      for (int k = 0 ; k < 2; k++) {
        EEPROM.write(m + addr, col[h].b[k]);
        m++; //m为字符串的个数
      }
    }
    EEPROM.write(addr + 1, m); //将WIFI名称的长度写入到存储到EPRROM里面，将用来前面的判断是否之前存储过WIFI
    m = 0; //将缓存清0
    for (int h = 0 ; h < WiFi.psk().length(); h++) //同理 密码
    {
      for (int k = 0 ; k < 2; k++) {
        EEPROM.write(4 + m, col_2[h].b[k]);
        m++;
      }
    }
    //    Serial.println("写入m得值为：" + m);
    EEPROM.write(0, m); //同理写入密码的长度
    
    EEPROM.commit();  //不要忘记储存！！！！
    //startTCPClient();
    wifiIsOk =1;
    server.end();
    Serial.println("restart");
    ESP.restart();
  }
}
void initDNS(void) { //初始化DNS服务器
  if (dnsServer.start(DNS_PORT, "*", apIP)) { //判断将所有地址映射到esp8266的ip上是否成功
    Serial.println("start dnsserver success.");
  }
  else Serial.println("start dnsserver failed.");
 
}
/**
* @发送请求指令
*/
bool sendRequestwt(const char* host, const char* cityid, const char* apiKey) {
  // We now create a URI for the request
  //心知天气
  String GetUrl = "/v3/weather/now.json?key=";
  GetUrl += apiKey;
  GetUrl += "&location=";
  GetUrl += city;
  GetUrl += "&language=";
  GetUrl += language;
  // This will send the request to the server
  client.print(String("GET ") + GetUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  DebugPrintln("create a request:");
  DebugPrintln(String("GET ") + GetUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n");
  delay(1000);
  return true;
}
bool sendRequestLife(const char* host, const char* cityid, const char* apiKey) {
  // We now create a URI for the request
  //心知天气
  String GetUrl = "/v3/life/suggestion.json?key=";
  GetUrl += apiKey;
  GetUrl += "&location=";
  GetUrl += city;
  GetUrl += "&language=";
  GetUrl += language;
  // This will send the request to the server
  client.print(String("GET ") + GetUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  DebugPrintln("create a request:");
  DebugPrintln(String("GET ") + GetUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n");
  delay(1000);
  return true;
}  
/**
* @Desc 跳过 HTTP 头，使我们在响应正文的开头
*/
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  bool ok = client.find(endOfHeaders);
  if (!ok) {
    DebugPrintln("No response or invalid response!");
  }
  return ok;
}
  
/**
* @Desc 从HTTP服务器响应中读取正文
*/
void readReponseContent(char* content, size_t maxSize) {
  size_t length = client.peekBytes(content, maxSize);
  delay(200);
  //Serial.println(length);
  DebugPrintln("Get the data from Internet!");
  content[length] = 0;
  DebugPrintln(content);
  DebugPrintln("Read data Over!");
  client.flush();//这句代码需要加上  不然会发现每隔一次client.find会失败
}
   
// 关闭与HTTP服务器连接
void stopConnect() {
  client.stop();
}
  
void clrEsp8266ResponseBuffer(void){
    memset(response, 0, MAX_CONTENT_SIZE);      //清空    
    memset(responseLife, 0, MAX_CONTENT_SIZE);      //清空
}
bool parseUserDataLife(char* content, struct UserDataLife* userData) {
  
  DynamicJsonDocument doc(1024); 
  
  auto error = deserializeJson(doc, content);
  if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return false;
  }  
  
  //复制我们感兴趣的字符串
  strcpy(userData->dressing, doc["results"][0]["suggestion"]["dressing"]["brief"]);  
  strcpy(userData->uv, doc["results"][0]["suggestion"]["uv"]["brief"]);
  //strcpy(userData->traffic, doc["results"][0]["suggestion"]["traffic"]["brief"]);
 // strcpy(userData->dating, doc["results"][0]["suggestion"]["dating"]["brief"]);
  strcpy(userData->sport, doc["results"][0]["suggestion"]["sport"]["brief"]);
  strcpy(userData->flu, doc["results"][0]["suggestion"]["flu"]["brief"]);
  //  -- 这不是强制复制，你可以使用指针，因为他们是指向“内容”缓冲区内，所以你需要确保
  //   当你读取字符串时它仍在内存中  
  return true;
}  
bool parseUserData(char* content, struct UserData* userData) {
//    -- 根据我们需要解析的数据来计算JSON缓冲区最佳大小
//   如果你使用StaticJsonBuffer时才需要
//    const size_t BUFFER_SIZE = 1024;
//   在堆栈上分配一个临时内存池
//    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
//    -- 如果堆栈的内存池太大，使用 DynamicJsonBuffer jsonBuffer 代替
//  DynamicJsonBuffer jsonBuffer;
  DynamicJsonDocument doc(1024); 
  
  auto error = deserializeJson(doc, content);
  if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return false;
  }  
  
  //复制我们感兴趣的字符串
  strcpy(userData->city, doc["results"][0]["location"]["name"]);
  strcpy(userData->weather, doc["results"][0]["now"]["text"]);
  strcpy(userData->temp, doc["results"][0]["now"]["temperature"]);  
  strcpy(userData->udate, doc["results"][0]["last_update"]);
  //  -- 这不是强制复制，你可以使用指针，因为他们是指向“内容”缓冲区内，所以你需要确保
  //   当你读取字符串时它仍在内存中  
  return true;
}
 
// 打印从JSON中提取的数据
void sendtoArduino(const struct UserData* userData) {  
  char s2Uno[128];
  String str1;
  int i;
  display.clear();  
  Serial.println("Wheather:[city] [wheather] [tempreture]");  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(6, 0, userData->city);
  display.drawString(66, 0, userData->weather);
  str1 = userData->temp;
  str1+="*C";
  display.drawString(6, 12,str1);
  timeClient.update();
  
  sprintf(s2Uno,"%02d : %02d",timeClient.getHours(),timeClient.getMinutes());  
  display.setFont(ArialMT_Plain_24); 
  display.drawString(28, 20, s2Uno);
 // sprintf(s2Uno,"%02d",); 
  display.setFont(ArialMT_Plain_10);
  i = timeClient.getDay();  
  display.drawString(0, 48, weekday[i]);  
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  int hours = timeClient.getHours();    //获取小时
  int minu =  timeClient.getMinutes();  //获取分钟
  int sece =  timeClient.getSeconds();  //获取秒
  Serial.printf("hour:%d minu:%d sece:%d\n", hours,minu,sece);
  display.display();
  //sprintf(s2Uno,"[%s][%s][%s]",userData->city,userData->weather,userData->temp);
  //Serial.println(s2Uno);
}
void sendtoArduinoLife(const struct UserDataLife* userData) {
  char s2Uno[256];  
  display.clear();
  Serial.println("Life:(dressing)(UV)(comfort)(car-wash)(travel)(flu)");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, userData->dressing);
  display.drawString(0, 16, userData->uv);
  display.drawString(0, 32, userData->flu);
  display.drawString(0, 48, userData->sport);
  //display.drawString(0, 48, userData->flu);
  display.display();
  //sprintf(s2Uno,"(%s)(%s)(%s)(%s)(%s)(%s)",userData->dressing,userData->uv,userData->sport,userData->car_washing,userData->travel,userData->flu);
  //Serial.println(s2Uno);
}
 
//初始化相关硬件
void setup() {
//  pinMode(LED_Pin, OUTPUT);
 
  pinMode(LED_Pin, OUTPUT);
  
  pinMode(BTN_Pin, INPUT);
  delay(100);
  Serial.begin(115200);
  wifiIsOk = 0;
  Serial.print("wifiIsOk = ");
  Serial.println(wifiIsOk);
  initBasic();
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(28, 32, "Hello ANT_Clock");
  display.display();
  EEPROM.begin(1024);  //申请512个缓存字节
 
  //  timeClient.begin();
  Serial.println("Beginning...");
  //首先判断是否之前已经缓存了一个WIFI密码和名称，如果没有则进行网页配网，并将WIFI信息进行储存
  //if (EEPROM.read(addr + 1) == 0 || EEPROM.read(0) == 0||digitalRead(BTN_Pin)==0) 
  if (digitalRead(BTN_Pin)==0) {
      //smart_config();//网页配网
    Serial.println("start AP");
    initSoftAP();
    initWebServer();
    initDNS();
    wifiIsOk = 0;
  }
  else   //如果之前已经储存过密码则对其进行读取
  {
    Serial.println("connet ssid");
    int n = 0;
    for (int i = 0 ; i < (int)EEPROM.read(addr + 1); i++) {
      if (i % 2 == 0) {
        col[30 + n].b[0] = EEPROM.read(addr + i);
        delay(200);
      }
      else
      {
        col[30 + n].b[1] = EEPROM.read(addr + i);
        //      Serial.print((char)col[30+n].a);
        get_ssid[n] = (char)col[30 + n].a ;
        n++;
        delay(200);
      }
    }
    n = 0;
 
    for (int i = 0 ; i < (int)EEPROM.read(0); i++) {
      if (i % 2 == 0) {
        col_2[30 + n].b[0] = EEPROM.read(4 + i);
        delay(200); //读取的时候记得延时合理的时间
      }
      else
      {
        col_2[30 + n].b[1] = EEPROM.read(4 + i);
        //      Serial.print((char)col_2[30+n].a);
        get_psk[n] = (char)col_2[30 + n].a ;
        n++;
        delay(200);
      }
    }
    
    String SSID_S(get_ssid);
    String psk_s(get_psk);
    
    Serial.println("字符串ssid: " + SSID_S);
    Serial.println("字符串secret: " + psk_s);    
    WiFi.setAutoConnect(true);
    WiFi.begin(SSID_S, psk_s);
    //WiFi.begin(ssid);*/
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      count++;
      if (count > 20) { //如果5秒内没有连上，就开启Web配网 可适当调整这个时间
        initSoftAP();
        initWebServer();
        initDNS();
        wifiIsOk = 0;
        break;
      }
    }
 
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
      Serial.println("IP address:");
      
      Serial.println(WiFi.localIP());
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(8, 0, "WiFi connected to");
      String SSID_S(get_ssid);
      display.drawString(8, 16, SSID_S);
      String sinfo = "IP address is:";      
      display.drawString(8, 32, sinfo);
      display.drawString(8, 48, WiFi.localIP().toString().c_str());
      display.display();
      wifiIsOk = 1;
      timeClient.begin();
      mslider.attachCallback([&](int value){
  Serial.println("[Card1] Slider Callback Triggered: "+String(value));
  lightR = value;
  mslider.update(value);
  dashboard.sendUpdates();
});
  mslider1.attachCallback([&](int value1){
  Serial.println("[Card1] Slider Callback Triggered: "+String(value1));
  lightG = value1;
  mslider1.update(value1);
  dashboard.sendUpdates();
});
  mslider2.attachCallback([&](int value2){
  Serial.println("[Card1] Slider Callback Triggered: "+String(value2));
  lightB = value2;
  mslider2.update(value2);
  dashboard.sendUpdates();
});
/* Attach Button Callback */
  button1.attachCallback([&](bool value){
    /* Print our new button value received from dashboard */
    Serial.println("Button Triggered: "+String((value)?"true":"false"));
    /* Make sure we update our button's value and send update to dashboard */
    //ledOn = 0;
    digitalWrite(LED_Pin, value);
    button1.update(value);
    dashboard.sendUpdates();
  });
  
  /* Start AsyncWebServer */
  server.begin();
  
  led.begin();
  led.clear();
  led.setBrightness(50); //设置亮度 (0~255)
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  //startTCPClient();
    }
  }
}
void startSTA() {
  WiFi.setAutoConnect(true);
  WiFi.begin(SSID_S, psk_s);
  delay(5000);
}
 
void loop() {
  //  Serial.println("IP address: ");
  int i;
  tcnt++;
  delay(1000);
  if(wifiIsOk==0)
  {
 
    dnsServer.processNextRequest();
    Serial.println("server run...");
    }else if(wifiIsOk==1){ 
      
     while (!client.connected()){
     if (!client.connect(host, 80)){
        // flag = !flag;
         DebugPrintln("connect to host ");
         delay(500);
     }
      
  }
  dashboard.sendUpdates();
  led.setPixelColor(0,led.Color(lightR, lightG, lightB)); //
  led.setPixelColor(1,led.Color(lightR, lightG, lightB)); //
  led.show(); //刷新显示
  /////////////////////////////////////
  if(tcnt==20)
  {
    tcnt=0;
    if (sendRequestwt(host, city, APIKEY) && skipResponseHeaders()) {
       clrEsp8266ResponseBuffer();
       readReponseContent(response, sizeof(response));
       UserData userData;
       if (parseUserData(response, &userData)) {
          sendtoArduino(&userData);
       }
      }
    }  
  }else if(wifiIsOk==2)
  {
        connectNewWifi();
    }
  ///////////////////////////////////
 
  if(digitalRead(BTN_Pin)==0)//设置按钮按下
  {
    i=0;
    while(!digitalRead(BTN_Pin))//长按5秒后重启进入设置页面
    {
       delay(1000);
       Serial.println(i);
       i++;
       if(i>5)
       ESP.restart();
      }
    }
  //Serial.println(digitalRead(BTN_Pin));
  Serial.print("wifiIsOk = ");
  Serial.println(wifiIsOk);
}
