#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#define DEBUG //是否启用debug功能
#include <SPI.h>             // Only needed for Arduino 1.6.5 and earlier

#include "SH1106Spi.h"
#ifdef DEBUG
#define DebugPrintln(message)    Serial.println(message)
#define DebugPrint(message)    Serial.print(message)
#else
#define DebugPrintln(message)
#define DebugPrint(message)
#endif



SH1106Spi display(D0, D2, D8);       // RES, DC , CS
const unsigned long BAUD_RATE = 115200;                   // serial connection speed
const unsigned long HTTP_TIMEOUT = 5000;               // max respone time from server
const size_t MAX_CONTENT_SIZE = 2048;                   // max size of the HTTP response
const char ssid[] = "xxxxxxxx";     //修改为自己的路由器用户名
const char password[] = "xxxxxxxx"; //修改为自己的路由器密码 
const char* host = "api.seniverse.com";
const char* APIKEY = "xxxxxxxx";        //心知天气网站提供的API KEY
const char* city = "beijing";
const char* language = "en";//zh-Hans 简体中文  会显示乱码
unsigned int cntFlag=0; 
WiFiClient client;
char response[MAX_CONTENT_SIZE];
char responseLife[MAX_CONTENT_SIZE];
char endOfHeaders[] = "\r\n\r\n"; 
unsigned int flag = HIGH;//默认当前灭灯
long lastTime = 0;
// 请求服务间隔
long Delay = 10000;
 
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
void connectWifi()
{  Serial.print("Connecting to " + *ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");  }
  Serial.println("");
  Serial.println("Connected");
  
}
/**
* @Desc 初始化操作
*/
void setup()   {                
  Serial.begin(BAUD_RATE);

  
  //pinMode(LED,OUTPUT);
  //digitalWrite(LED, HIGH);
  connectWifi();
  DebugPrintln("IP address: ");
  DebugPrintln(WiFi.localIP());//WiFi.localIP()返回8266获得的ip地址
  lastTime = millis();
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Hello ANT_Clock");
  display.display();
}
 
void loop() {
  while (!client.connected()){
     if (!client.connect(host, 80)){
         flag = !flag;
         DebugPrintln("connect to host ");
         delay(500);
     }
  }
 
  if(millis()-lastTime>=Delay){
         //每间隔20s左右调用一次  
         cntFlag = !cntFlag;
        Serial.println( cntFlag);
     lastTime = millis();
     if(cntFlag==1)//读取天气
     {
     
      if (sendRequestwt(host, city, APIKEY) && skipResponseHeaders()) {
       clrEsp8266ResponseBuffer();
       readReponseContent(response, sizeof(response));
       UserData userData;
       if (parseUserData(response, &userData)) {
          sendtoArduino(&userData);
       }
      }
      }else if(cntFlag==0){//读取生活指数
        
        if (sendRequestLife(host, city, APIKEY) && skipResponseHeaders()) {
       clrEsp8266ResponseBuffer();
       readReponseContent(responseLife, sizeof(responseLife));
       UserDataLife userData;
       if (parseUserDataLife(responseLife, &userData)) {
          sendtoArduinoLife(&userData);
       }
      }
        }    
  }
  
  
  
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
  display.clear();  
  Serial.println("Wheather:[city] [wheather] [tempreture]");  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, userData->city);
  display.drawString(0, 24, userData->weather);
  display.drawString(0, 48, userData->temp);
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
