#include <ABTKITS.h>
#include <dht11.h>
#define NOTE_CS5 554 //3
#define NOTE_D5  587 //4
#define NOTE_E5  659 //5
#define DHT11PIN 6//温湿度数据管脚
dht11 DHT11;
ABTKITS abtKits;
int stuD4=1;
int stuD5=1;
void setup() {
  // put your setup code here, to run once:
  abtKits.ABTINIT();
  delay(200);
  pinMode(4,INPUT);
  pinMode(5,INPUT);
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(13,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int i;
  char scmd[32];
  i = digitalRead(4);
  if(stuD4!=i)
  {
    stuD4 = i;
    sprintf(scmd,"ABTDR04,%d#",stuD4);
    sendStringToFlash(scmd);    
    }
  i = digitalRead(5);
  if(stuD5!=i)
  {
    stuD5 = i;
    sprintf(scmd,"ABTDR05,%d#",stuD5);
    sendStringToFlash(scmd);    
    }
  int chk = DHT11.read(DHT11PIN);//读温湿度
  if(chk==DHTLIB_OK)
    {
        sprintf(scmd,"ABTSR00,%d#",DHT11.temperature);//温度
        sendStringToFlash(scmd);delay(20);
        sprintf(scmd,"ABTSR01,%d#",DHT11.humidity);//湿度
        sendStringToFlash(scmd);delay(20);     
    }
  abtKits.ABTGetBleCmd();
  delay(20);
  abtKits.ABTHandleBleCmd();
   if(abtKits.curInfo.cRW=='W')
    {
      abtKits.curInfo.cRW = 'N';//避免下一次重复进入
      if(abtKits.curInfo.sID==7)
      {
        WarnSound();
        }
    }
  delay(100);
}
void sendStringToFlash (char *s)
{
  while (*s) {
    Serial.write(*s ++);
  }
  Serial.write(0);
}
void WarnSound()
{
  int tonePin = 7;  
  int melody[] = {
NOTE_CS5,//
NOTE_D5,//
NOTE_D5,//
NOTE_CS5,//
0,
};

int noteDurations[] = {
  8,8,4,8,
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
