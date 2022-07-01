#include <Arduino.h>

# define ESP8266
#if defined(ESP8266)
  /* ESP8266 Dependencies */
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#elif defined(ESP32)
  /* ESP32 Dependencies */
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#endif
#include <ESPDash.h>

#include <Adafruit_NeoPixel.h>

#define PIN        0  //GPIO0接口
#define NUMPIXELS  1  //彩灯个数

Adafruit_NeoPixel led(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
/* Your WiFi Credentials */
const char* ssid = "******"; // SSID
const char* password = "********"; // Password

/* Start Webserver */
AsyncWebServer server(80);

/* Attach ESP-DASH to AsyncWebServer */
ESPDash dashboard(&server); 

/* 
  Dashboard Cards 
  Format - (Dashboard Instance, Card Type, Card Name, Card Symbol(optional) )
*/
//Card temperature(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
//Card humidity(&dashboard, HUMIDITY_CARD, "Humidity", "%");
Card mslider(&dashboard, SLIDER_CARD, "LED Red", "", 0, 255);
Card mslider1(&dashboard, SLIDER_CARD, "LED Green", "", 0, 255);
Card mslider2(&dashboard, SLIDER_CARD, "LED Blue", "", 0, 255);
/*mslider.attachCallback([&](int value){
  Serial.println("[Card1] Slider Callback Triggered: "+String(value));
  card1.update(value);
  dashboard.sendUpdates();
});*/

int lightR=100;
int lightG=100;
int lightB=100;
void setup() {
  Serial.begin(115200);  
  /* Connect WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      return;
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
  /* Start AsyncWebServer */
  server.begin();
  led.begin();
  led.clear();
  led.setBrightness(80); //设置亮度 (0~255)
  
}

void loop() {
  /* Update Card Values */
  
  //temperature.update((int)random(0, 50));
  //humidity.update((int)random(0, 100));
  
  /* Send Updates to our Dashboard (realtime) */
  dashboard.sendUpdates();

  /* 
    Delay is just for demonstration purposes in this example,
    Replace this code with 'millis interval' in your final project.
  */
  delay(1000);
  led.setPixelColor(0,led.Color(lightR, lightG, lightB)); //
  led.show(); //刷新显示
  delay(1000);
}
