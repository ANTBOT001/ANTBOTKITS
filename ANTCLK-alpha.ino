#include <Arduino.h>
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SH1106Wire.h" 
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
//Settings
#define SLOW_BOOT 0
#define HOSTNAME "ANTCLKWIFISETUI"
#define FORCE_USE_HOTSPOT 0
#define PIN        0  //GPIO0接口
#define NUMPIXELS  4  //彩灯个数
#define BTN_Pin D7
#define SWT_Pin D0
#define NTD1 294
#define NTD2 330
#define NTD3 350
int tune1[]=
{
  NTD1,NTD3
};
int tune2[]=
{
  NTD3,NTD1
};
float durt[]=
{
  0.25,0.25,0.25
};
int songLength=2;
int tonepin=15;
Adafruit_NeoPixel led(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com",60*60*8, 30*60*1000);
String weekday[8] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday",""};

//const char* ssid = "ESPUI";
//const char* password = "espui";
//const char* hostname = "espui";

uint16_t button1;
uint16_t switchOne;
uint16_t switchTwo;
uint16_t status;
uint16_t r_slid;
uint16_t g_slid;
uint16_t b_slid;
uint16_t l_slid;
uint16_t text_time;
uint16_t deftemp;
uint16_t defhumi;
//UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
int wifiIsok = 0;
SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL
int bellFlag = 0;//clock on/off
int led_r=0;
int led_g=0;
int led_b=0;
int led_l=0;   //lightness ws2812
int led_s=0;   //light on/off
String settime;
int clockbellflag = 0;

void Bell(int tt)
{
  int x;
  if(tt)
  {
    for(x=0;x<songLength;x++)
  {
    tone(tonepin,tune1[x]);    
    delay(400*durt[x]);    
    delay(100*durt[x]);
    noTone(tonepin);
  }
    }else{
      for(x=0;x<songLength;x++)
  {
    tone(tonepin,tune2[x]);    
    delay(400*durt[x]);    
    delay(100*durt[x]);
    noTone(tonepin);
  }
      }
  
}

void readStringFromEEPROM(String& buf, int baseaddress, int size) {
  buf.reserve(size);
  for (int i = baseaddress; i < baseaddress+size; i++) {
    char c = EEPROM.read(i);
    buf += c;
    if(!c) break;
  } 
}

void connectWifi() {
  int connect_timeout;

#if defined(ESP32)
  WiFi.setHostname(HOSTNAME);
#else
  WiFi.hostname(HOSTNAME);
#endif
  Serial.println("Begin wifi...");

  //Load credentials from EEPROM 
  if(!(FORCE_USE_HOTSPOT)) {
    yield();
    EEPROM.begin(100);
    String stored_ssid, stored_pass;
    readStringFromEEPROM(stored_ssid, 0, 32);
    readStringFromEEPROM(stored_pass, 32, 96);
    EEPROM.end();
  
    //Try to connect with stored credentials, fire up an access point if they don't work.
    #if defined(ESP32)
      WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
    #else
      WiFi.begin(stored_ssid, stored_pass);
    #endif
    connect_timeout = 28; //7 seconds
    while (WiFi.status() != WL_CONNECTED && connect_timeout > 0) {
      delay(250);
      Serial.print(".");
      connect_timeout--;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
    Serial.println("Wifi started");
    wifiIsok = 1;
    if (!MDNS.begin(HOSTNAME)) {
      Serial.println("Error setting up MDNS responder!");
    }
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  } else {
    Serial.println("\nCreating access point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(HOSTNAME);

    connect_timeout = 20;
    do {
      delay(250);
      Serial.print(",");
      connect_timeout--;
    } while(connect_timeout);
  }
}

void enterWifiDetailsCallback(Control *sender, int type) {
  if(type == B_UP) {
    Serial.println("Saving credentials to EPROM...");
    Serial.println(ESPUI.getControl(wifi_ssid_text)->value);
    Serial.println(ESPUI.getControl(wifi_pass_text)->value);
    unsigned int i;
    EEPROM.begin(100);
    for(i = 0; i < ESPUI.getControl(wifi_ssid_text)->value.length(); i++) {
      EEPROM.write(i, ESPUI.getControl(wifi_ssid_text)->value.charAt(i));
      if(i==30) break; //Even though we provided a max length, user input should never be trusted
    }
    EEPROM.write(i, '\0');

    for(i = 0; i < ESPUI.getControl(wifi_pass_text)->value.length(); i++) {
      EEPROM.write(i + 32, ESPUI.getControl(wifi_pass_text)->value.charAt(i));
      if(i==94) break; //Even though we provided a max length, user input should never be trusted
    }
    EEPROM.write(i + 32, '\0');
    EEPROM.end();
    wifiIsok = 2;
  }
}
void getUIdata()
{
/*  uint16_t switchOne;
uint16_t switchTwo;
uint16_t status;
uint16_t r_slid;
uint16_t g_slid;
uint16_t b_slid;
uint16_t l_slid;
uint16_t text_time;*/
int i;
static int obellFlag = 0;//clock on/off
static int oled_r=0;
static int oled_g=0;
static int oled_b=0;
static int oled_l=0;   //lightness ws2812
static int oled_s=0;   //light on/off
Serial.print("switchOne: ");
Serial.println(ESPUI.getControl(switchOne)->value);
Serial.print("switchTwo: ");
Serial.println(ESPUI.getControl(switchTwo)->value);
bellFlag = ESPUI.getControl(switchOne)->value.toInt();
led_s    = ESPUI.getControl(switchTwo)->value.toInt();

settime  = ESPUI.getControl(text_time)->value;
Serial.println(settime);
if(oled_s != led_s)
{
  digitalWrite(SWT_Pin, led_s);
  Bell(led_s);
  }
  led_r = ESPUI.getControl(r_slid)->value.toInt();
  led_g = ESPUI.getControl(g_slid)->value.toInt();
  led_b = ESPUI.getControl(b_slid)->value.toInt();
  led_l = ESPUI.getControl(l_slid)->value.toInt();
  if(led_r!=oled_r||led_g!=oled_g||led_b!=oled_b)
  {
    for(i=0;i<NUMPIXELS;i++)
    {
      led.setPixelColor(i,led.Color(led_r, led_g, led_b)); //  
      led.show();
      delay(100); 
      }
    Serial.print("set led ");
    Serial.print(led_r);
    Serial.print(led_g);
    Serial.println(led_b);
    }
   if(led_l!=oled_l)
   {
    led.setBrightness(led_l); //设置亮度 (0~255)
    led.show(); 
    Serial.print("set light ");
    Serial.println(led_l);
    }
oled_s = led_s;
obellFlag = bellFlag;
oled_r = led_r;
oled_g = led_g;
oled_b = led_b;
oled_l = led_l;
  }
void oledDisplay() {  
  char s2Uno[128];
  char clktime[128];
  String str1;
  int i;
  int shour;
  int smin;
  int ssec;
  int rtemp=22;
  int rhumi=40;
  display.clear();  
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  sprintf(s2Uno,"temp:%02d*C",rtemp);
  display.drawString(0, 8, s2Uno);
  sprintf(s2Uno,"humi:%02d%%",rhumi);
  display.drawString(72, 8, s2Uno);      
  timeClient.update();
  shour = timeClient.getHours();
  smin = timeClient.getMinutes();
  ssec = timeClient.getSeconds();
  sprintf(s2Uno,"%02d : %02d",shour,smin);  
  
  display.setFont(ArialMT_Plain_24); 
  display.drawString(28, 20, s2Uno); 
  display.setFont(ArialMT_Plain_10);
  i = timeClient.getDay();  
  display.drawString(0, 48, weekday[i]);
  display.drawString(64, 48,settime);    
  if(bellFlag)
  {
    sprintf(clktime,"%s",settime);
    Serial.println(clktime);
    Serial.println(atoi(clktime));
    Serial.println(atoi(clktime+3));
    if(shour==atoi(clktime)&&smin==atoi(clktime+3)&&ssec<30)
    {
      clockbellflag=1;
      Serial.println("bell is trigger!");
      }
    
    display.drawString(94, 48, "ON ");
    }  
  else
  {
    display.drawString(94, 48, "OFF");  
    }
  
  display.display();
  //sprintf(s2Uno,"[%s][%s][%s]",userData->city,userData->weather,userData->temp);
  //Serial.println(s2Uno);
}
void textCallback(Control *sender, int type) {
  //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

void randomString(char *buf, int len) {
  for(auto i = 0; i < len-1; i++) 
    buf[i] = random(0, 26) + 'A';
  buf[len-1] = '\0';
}
void numberCall(Control* sender, int type)
{
    Serial.println(sender->value);
}

void textCall(Control* sender, int type)
{
    Serial.print("Text: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
}

void slider(Control* sender, int type)
{
    Serial.print("Slider: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
}



void switchExample(Control* sender, int value)
{
    switch (value)
    {
    case S_ACTIVE:
        Serial.print("Active:");
        break;

    case S_INACTIVE:
        Serial.print("Inactive");
        break;
    }

    Serial.print(" ");
    Serial.println(sender->id);
}

void otherSwitchExample(Control* sender, int value)
{
    switch (value)
    {
    case S_ACTIVE:
        Serial.print("Active:");
        break;

    case S_INACTIVE:
        Serial.print("Inactive");
        break;
    }

    Serial.print(" ");
    Serial.println(sender->id);
}
void SetUILink()
{
  auto wifitab = ESPUI.addControl(Tab, "", "WiFi Credentials");
  wifi_ssid_text = ESPUI.addControl(Text, "SSID", "", Alizarin, wifitab, textCallback);
  //Note that adding a "Max" control to a text control sets the max length
  ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
  wifi_pass_text = ESPUI.addControl(Text, "Password", "", Alizarin, wifitab, textCallback);
  ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
  ESPUI.addControl(Button, "Save", "Save", Peterriver, wifitab, enterWifiDetailsCallback);


  //Finally, start up the UI. 
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(HOSTNAME);
  }
void SetUIUser()
{
      uint16_t tab1 = ESPUI.addControl(ControlType::Tab, "Red", "Red");
    uint16_t tab2 = ESPUI.addControl(ControlType::Tab, "Green", "Green");
    uint16_t tab3 = ESPUI.addControl(ControlType::Tab, "Blue", "Blue");
    uint16_t tab4 = ESPUI.addControl(ControlType::Tab, "Light", "Light");
    
    
    // shown above all tabs
     deftemp = ESPUI.addControl(ControlType::Button, "Temp&Humi", "22*C", ControlColor::Turquoise,Control::noParent);    
     defhumi= ESPUI.addControl(ControlType::Button, "", " 40% ", ControlColor::Emerald, deftemp);
  ESPUI.setPanelStyle(deftemp, "background: linear-gradient(90deg, rgba(131,58,180,1) 0%, rgba(253,29,29,1) 50%, rgba(252,176,69,1) 100%); border-bottom: #555;");
  ESPUI.setElementStyle(deftemp, "border-radius: 2em; border: 3px solid black; width: 30%; background-color: #8df;");
  ESPUI.setPanelStyle(defhumi, "background: linear-gradient(90deg, rgba(131,58,180,1) 0%, rgba(253,29,29,1) 50%, rgba(252,176,69,1) 100%); border-bottom: #555;");
  ESPUI.setElementStyle(defhumi, "border-radius: 2em; border: 3px solid black; width: 30%; background-color: #8df;");

           //ESPUI.setPanelWide(status, true);
    r_slid = ESPUI.addControl(ControlType::Slider, "Red", "100", ControlColor::Alizarin, tab1, &slider);
    g_slid = ESPUI.addControl(ControlType::Slider, "Green", "100", ControlColor::Emerald, tab2, &slider);
    b_slid = ESPUI.addControl(ControlType::Slider, "Blue", "100", ControlColor::Peterriver, tab3, &slider);
    l_slid = ESPUI.addControl(ControlType::Slider, "Light", "100", ControlColor::Sunflower, tab4, &slider);
    ESPUI.addControl(Min, "", "0", None, r_slid);
    ESPUI.addControl(Max, "", "255", None, r_slid);
     ESPUI.addControl(Min, "", "0", None, g_slid);
    ESPUI.addControl(Max, "", "255", None, g_slid);
     ESPUI.addControl(Min, "", "0", None, b_slid);
    ESPUI.addControl(Max, "", "255", None, b_slid);
     ESPUI.addControl(Min, "", "0", None, l_slid);
    ESPUI.addControl(Max, "", "255", None, l_slid);
    
    ESPUI.sliderContinuous = true;
    text_time = ESPUI.text("Set Clock Time", &textCall, ControlColor::Carrot, "06:00");
    ESPUI.setInputType(text_time, "time");
    ESPUI.addControl(ControlType::Max, "", "32", ControlColor::None, text_time,&textCall);

    switchOne = ESPUI.addControl(ControlType::Switcher, "", "0", ControlColor::None,text_time, &switchExample);
    switchTwo = ESPUI.addControl(ControlType::Switcher, "Light ON/OFF", "0", ControlColor::Dark, Control::noParent,&switchExample);
    ESPUI.begin("ESPUI Control");
  } 
void setup(void)
{

  pinMode(SWT_Pin, OUTPUT);
  pinMode(tonepin,OUTPUT);
  pinMode(BTN_Pin, INPUT);
    Serial.begin(115200);

    delay(2000);
    display.init();
    display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(28, 32, "Hello ANT_Clock");
    display.display();
    connectWifi();
    digitalWrite(SWT_Pin, 1);
    delay(500);
    led.begin();
    //led.clear();
    led.setBrightness(100); //设置亮度 (0~255)
    delay(1000);
    if(wifiIsok)
    {
       SetUIUser();       
        display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(8, 0, "now connected to");    
    display.drawString(8, 16, WiFi.SSID());
    display.drawString(8, 32, WiFi.localIP().toString().c_str());
    display.drawString(8, 48, "open this ip , enjoy it");  
    display.display();
      }else{
       SetUILink();
        display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(8, 0, "setup your wifi...");
    display.drawString(8,16, "please find ssid:");
    display.drawString(8,32, "ANTCLKWIFISETUI");
    display.drawString(8,48, "and connect this ssid");
    display.display();
    
    
    //digitalWrite(SWT_Pin, 0);
        }
    
    
}

void loop(void)
{
  static int i=0;
  dnsServer.processNextRequest();
   Serial.print("wifiisok = ");
   Serial.println(wifiIsok);
   if(i==3)
   {
    i=0;
        if(wifiIsok==2)
        {
            ESP.restart();
         }else if(wifiIsok==1)
         {
            oledDisplay();
            getUIdata();
            ESPUI.updateButton(deftemp,"22*C");
            ESPUI.updateButton(defhumi,"40%");
          }
    }
   
     if(digitalRead(BTN_Pin)==0)//设置按钮按下
  {
    delay(200);
    if(digitalRead(BTN_Pin)==0)
    {
      clockbellflag =0;
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(8, 0, "now connected to");    
      display.drawString(8, 16, WiFi.SSID());
      display.drawString(8, 32, WiFi.localIP().toString().c_str());
      display.drawString(8, 48, "open this ip , enjoy it");  
      display.display();
      delay(5000);
      }
      
    }
   if(clockbellflag)
   {
        Bell(0);
        delay(200);
         Bell(1);
    }
   delay(1000);
   i++;
}
