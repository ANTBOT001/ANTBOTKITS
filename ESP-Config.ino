#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>


#define LED_Pin D4
#define BTN_Pin D7
String my_led_status = "off";

char get_ssid[40] = {0}; //从EEPROM中读取得到的SSIDchar类型数组
char get_psk[50] = {0}; //从EEPROM中读取得到的WIFI密码

String SSID_S;
String psk_s;
#define addr 200   //这个地址可以随意设置，如果用本程序则不能设置为小于50以下的值（为读取密码字符串预留空间）
#define addr1 400
#define addr2 600
#define addr3 740

#define MAX_PACKETSIZE 512

WiFiClient TCPclient;
bool preTCPConnected = false;

//led控制函数，具体函数内容见下方
void turnOnLed();
void turnOffLed();
int wifiIsOk=0;
int m ; //读取字符串长度的缓冲变量
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
ESP8266WebServer server(80);//创建WebServer
 
void handleRoot() {//访问主页回调函数
  server.send(200, "text/html", page_html);
  if (server.arg("btn1") == "1") {
    ESP.restart();
    server.send(200, "text/html", "<meta charset='UTF-8'>重启成功");//返回保存成功页面
  }
}
 
 
void handleRootPost() {//Post回调函数
  Serial.println("handleRootPost");
 
 
  if (server.hasArg("ssid")) {//判断是否有账号参数
    Serial.print("got ssid:");
    strcpy(get_ssid, server.arg("ssid").c_str());//将账号参数拷贝到get_ssid中
    Serial.println(get_ssid);
 
  } else {//没有参数
    Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");//返回错误页面
    return;
  }
  //密码与账号同理
  if (server.hasArg("password")) {
    Serial.print("got password:");
    strcpy(get_psk, server.arg("password").c_str());
    Serial.println(get_psk);
  } else {
    Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }  
 
  server.send(200, "text/html", "<meta charset='UTF-8'>保存成功");//返回保存成功页面
 
  delay(2000);
  //连接wifi
  connectNewWifi();
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
  }
}
 
void initWebServer(void) { //初始化WebServer
  //server.on("/",handleRoot);
  //上面那行必须以下面这种格式去写否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);//设置主页回调函数
  server.onNotFound(handleRoot);//设置无法响应的http请求的回调函数
  server.on("/", HTTP_POST, handleRootPost);//设置Post请求回调函数
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
    if (count > 50) { //如果5秒内没有连上，就开启Web配网 可适当调整这个时间
      initSoftAP();
      initWebServer();
      initDNS();
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
    server.stop();
  }
}
void initDNS(void) { //初始化DNS服务器
  if (dnsServer.start(DNS_PORT, "*", apIP)) { //判断将所有地址映射到esp8266的ip上是否成功
    Serial.println("start dnsserver success.");
  }
  else Serial.println("start dnsserver failed.");
 
}
 
 
//初始化相关硬件
void setup() {
//  pinMode(LED_Pin, OUTPUT);
//  digitalWrite(LED_Pin, HIGH);
  pinMode(BTN_Pin, INPUT);
  delay(100);
  Serial.begin(115200);
  initBasic();
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
      if (count > 100) { //如果5秒内没有连上，就开启Web配网 可适当调整这个时间
        initSoftAP();
        initWebServer();
        initDNS();
        break;
      }
    }
 
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
      Serial.println("IP address:");
      Serial.println(WiFi.localIP());
      wifiIsOk = 1;
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
  if(!wifiIsOk)
  {
    server.handleClient();
    dnsServer.processNextRequest();
    } 
  
  delay(1000);
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
  Serial.println(digitalRead(BTN_Pin));
}

