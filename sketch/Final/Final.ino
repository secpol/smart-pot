#include <WiFi.h>
#include <EEPROM.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"

#define DHT_OK      1
#define DHT_ERR_CHECK 0
#define DHT_ERR_TIMEOUT -1


char* APname ="ZZULI-TEST";
char* APpass = "12345678";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  initSDcard();
  start_server(initWifi());
}

void loop(){
  write_log(getTime(read_NTP_EEPROM(),read_timeZone_EEPROM()).substring(0,8),sensorData());
  delay(60000);
}

void read_plant_eeprom(String* tempH,String* tempL,String* lightH,String* lightL,String* soilH,String* soilL,String* waterT){
  *tempH="";
  *tempL="";
  *lightH="";
  *lightL="";
  *soilH="";
  *soilL="";
  *waterT="";
  for (int i = 359; i < 367; i++)
  {
    *tempH += char(EEPROM.read(i));
  }
  for (int i = 367; i < 375; i++)
  {
    *tempL += char(EEPROM.read(i));
  }
  for (int i = 375; i < 383; i++)
  {
    *lightH += char(EEPROM.read(i));
  }
  for (int i = 383; i < 391; i++)
  {
    *lightL += char(EEPROM.read(i));
  }
  for (int i = 391; i < 399; i++)
  {
    *soilH += char(EEPROM.read(i));
  }
  for (int i = 399; i < 407; i++)
  {
    *soilL += char(EEPROM.read(i));
  }
  for (int i = 407; i < 415; i++)
  {
    *waterT += char(EEPROM.read(i));
  }
}

int initWifi(){
  if(read_ssid_EEPROM().length()>1){
    WiFi.mode(WIFI_STA);
    WiFi.begin(read_ssid_EEPROM().c_str(),read_pass_EEPROM().c_str());
    Serial.print(read_ssid_EEPROM());
    Serial.println();
    Serial.print(read_pass_EEPROM());
    delay(10000);
    if(WiFi.status() == WL_CONNECTED)
    {
      server.begin();
      return 0;
    }
  }
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APname,APpass,6);
    Serial.println("AP");
    server.begin();
    return 1;
}

void start_server(int type){
  if(type == 0){  //STA-Webserver
    
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
    String arg =(request->getParam(0)->name());
      if(arg.equals("date")){
        String date=(request->getParam(0)->value());
        String content;
        if(!read_log(date,&content)){
          request->send(200, "application/json",content);
        }else{
          request->send(404);
        }
      }else{
        request->send(404);
      }
  });

    server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request){
    bool hasPPass=false;
    String isAuto="";
    String NTP="";
    String timeZone="";
    String ppass="";
    for(int i=0;i<request->params();i++){
      String arg =(request->getParam(i)->name());
      if(arg.equals("isAuto")){
        isAuto="";
        isAuto+=(request->getParam(i)->value());
      }else if(arg.equals("NTP")){
        NTP="";
        NTP+=(request->getParam(i)->value());
      }else if(arg.equals("timeZone")){
        timeZone="";
        timeZone+=(request->getParam(i)->value());
      }else if(arg.equals("ppass")){
        hasPPass = true;
        ppass="";
        ppass+=(request->getParam(i)->value());
      }
    }
    if(hasPPass && read_ppass_EEPROM().equals(ppass)){
      write_settings_eeprom(isAuto,NTP,timeZone);
      request->send(200);
    }else{
      request->send(400);
    }
  });

    server.on("/plant", HTTP_POST, [](AsyncWebServerRequest *request){
    bool hasPPass=false;
    String tempH="";
    String tempL="";
    String lightH="";
    String lightL="";
    String soilH="";
    String soilL="";
    String waterT="";
    String humidity="";
    String ppass="";

    for(int i=0;i<request->params();i++){
      String arg =(request->getParam(i)->name());
      if(arg.equals("tempH")){
        tempH="";
        tempH+=(request->getParam(i)->value());
      }else if(arg.equals("tempL")){
        tempL="";
        tempL+=(request->getParam(i)->value());
      }else if(arg.equals("lightH")){
        lightH="";
        lightH+=(request->getParam(i)->value());
      }else if(arg.equals("lightL")){
        lightL="";
        lightL+=(request->getParam(i)->value());
      }else if(arg.equals("soilH")){
        soilH="";
        soilH+=(request->getParam(i)->value());
      }else if(arg.equals("soilL")){
        soilL="";
        soilL+=(request->getParam(i)->value());
      }else if(arg.equals("waterT")){
        waterT="";
        waterT+=(request->getParam(i)->value());
      }else if(arg.equals("humidity")){
        humidity="";
        humidity+=(request->getParam(i)->value());
      }else if(arg.equals("ppass")){
        hasPPass = true;
        ppass="";
        ppass+=(request->getParam(i)->value());
      }
    }
    if(hasPPass && 
       ppass.length()==6 && 
       tempH.length()!=0 &&
       tempL.length()!=0 &&
       lightH.length()!=0 &&
       lightL.length()!=0 &&
       soilH.length()!=0 &&
       soilL.length()!=0 &&
       waterT.length()!=0 &&
       humidity.length()!=0){
      write_plant_eeprom(tempH,tempL,lightH,lightL,soilH,soilL,waterT);
      request->send(200);
      ESP.restart();
    }else{
      request->send(400);
    }
  });

  }
  else{   //AP-Webserver
    
    server.on("/ap", HTTP_POST, [](AsyncWebServerRequest *request){
    bool hasPPass=false;
    String ssid="";
    String pass="";
    String ppass="";
    for(int i=0;i<request->params();i++){
      String arg =(request->getParam(i)->name());
      if(arg.equals("ssid")){
        ssid="";
        ssid+=(request->getParam(i)->value());
      }else if(arg.equals("pass")){
        pass="";
        pass+=(request->getParam(i)->value());
        if(pass.length()<8) pass="";
      }else if(arg.equals("ppass")){
        hasPPass = true;
        ppass="";
        ppass+=(request->getParam(i)->value());
      }
    }
    if(hasPPass && ppass.length()==6){
      write_wifi_eeprom(ssid,pass,ppass);
      Serial.println(ssid);
      Serial.println(pass);
      Serial.println(ppass);
      request->send(200);
      ESP.restart();
    }else{
      request->send(400);
    }
  });
  
  }
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"MAC\":\"30AEA44575CC\",\"ChipID\":\"CC7545A4AE30\"}");
  });
  
  server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request){
      String arg =(request->getParam(0)->name());
      if(arg.equals("ppass")){
        String ppassvalue=(request->getParam(0)->value());
        if(ppassvalue.equals(read_ppass_EEPROM())){
          write_wifi_eeprom("","","");
          write_settings_eeprom("0","pool.ntp.org","0");
          request->send(200);
          ESP.restart();
        }
      }else{
        request->send(400);
      }
  });


}

String read_ssid_EEPROM(){
  String ssid;
  for (int i = 0; i < 32; i++)
  {
    ssid += char(EEPROM.read(i));
  }
  return ssid;
}

String read_pass_EEPROM(){
  String pass;
  for (int i = 32; i < 96; i++)
  {
    pass += char(EEPROM.read(i));
  }
  if(pass.length()<8){
    pass="";
  }
  return pass;
}

String read_ppass_EEPROM(){
  String ppass;
  for (int i = 96; i < 102; i++)
  {
    ppass += char(EEPROM.read(i));
  }
  return ppass;
}

String read_NTP_EEPROM(){
  String NTP;
  for (int i = 104; i < 358; i++)
  {
    NTP += char(EEPROM.read(i));
  }
  return NTP;
}

String read_timeZone_EEPROM(){
  String timeZone;
  for (int i = 104; i < 358; i++)
  {
    timeZone += char(EEPROM.read(i));
  }
  return timeZone;
}

void write_wifi_eeprom(String ssid, String pass, String ppass){
  for (int i = 0; i < 32; i++)
  {
    EEPROM.write(i,ssid.charAt(i));
  }
  for (int i = 32; i < 96; i++)
  {
    EEPROM.write(i,pass.charAt(i-32));
  }
  for (int i = 96; i < 102; i++)
  {
    EEPROM.write(i,ppass.charAt(i-96));
  }
  EEPROM.commit();
}

void write_settings_eeprom(String isAuto, String NTP, String timeZone){
  for (int i = 102; i < 104; i++)
  {
    EEPROM.write(i,isAuto.charAt(i-102));
  }
  for (int i = 104; i < 352; i++)
  {
    EEPROM.write(i,NTP.charAt(i-104));
  }
  for (int i = 352; i < 359; i++)
  {
    EEPROM.write(i,timeZone.charAt(i-352));
  }
  EEPROM.commit();
}

void write_plant_eeprom(String tempH,String tempL,String lightH,String lightL,String soilH,String soilL,String waterT){
  for (int i = 359; i < 367; i++)
  {
    EEPROM.write(i,tempH.charAt(i-359));
  }
  for (int i = 367; i < 375; i++)
  {
    EEPROM.write(i,tempL.charAt(i-362));
  }
  for (int i = 375; i < 383; i++)
  {
    EEPROM.write(i,lightH.charAt(i-359));
  }
  for (int i = 383; i < 391; i++)
  {
    EEPROM.write(i,lightL.charAt(i-359));
  }
  for (int i = 391; i < 399; i++)
  {
    EEPROM.write(i,soilH.charAt(i-359));
  }
  for (int i = 399; i < 407; i++)
  {
    EEPROM.write(i,soilL.charAt(i-359));
  }
  for (int i = 407; i < 415; i++)
  {
    EEPROM.write(i,waterT.charAt(i-359));
  }
  EEPROM.commit();
}

String sensorData() {
  int power;
  int soil;
  float temp;
  float humidity;
  int waterlevel;
  int light;

  while(DHT_read(&humidity, &temp)!=DHT_OK){}
  
  pinMode(A0,INPUT);  //soil
  pinMode(A3,INPUT);  //light
  pinMode(A6,INPUT);  //power
  pinMode(A7,INPUT);  //waterLevel1-5
  pinMode(A4,INPUT);
  pinMode(A5,INPUT);
  pinMode(A18,INPUT);
  pinMode(A19,INPUT);
  pinMode(14,OUTPUT); //led
  pinMode(12,OUTPUT); //pump

  soil = analogRead(A0)*100/4095;
  light = analogRead(A3)*100/4095;
  power = analogRead(A6)*100/4095;
  waterlevel = 5;
  
    String datalog = "\n{\n";
    datalog += "\"time\": \"" + getTime(read_NTP_EEPROM(),read_timeZone_EEPROM()).substring(8) + "\",\n";
    datalog += "\"power\": \"" + String(power) + "\",\n";
    datalog += "\"soil\": \"" + String(soil) + "\",\n";
    datalog += "\"temp\": \"" + String(temp) + "\",\n";
    datalog += "\"humidity\": \"" + String(humidity) + "\",\n";
    datalog += "\"waterlevel\": \"" + String(waterlevel) + "\",\n";
    datalog += "\"light\": \"" + String(light) + "\"\n";
    datalog += "},";
  return datalog;
}

int initSDcard(){
  if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return 1;
    }
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        return 1;
    }
    SD.mkdir("/log");
    return 0;
}

int read_log(String date,String* content){
  Serial.println("Data read:");
  Serial.println(date);
  String path = "/log/";
  path += date;
  path += ".txt";
  String temp="{\n\"data\": [";
  
    File file = SD.open(path.c_str());
    if(!file){
        return 1;
    }
    while(file.available()){
        temp+= (char)file.read();
    }
    temp = temp.substring(0,temp.length()-1);
    temp += "]}";
    *content = temp;
    file.close();
    return 0;
}

int write_log(String date,String content){
   Serial.println("Data write:");
   Serial.println(content);
  if(space_use_test(80)){
    delete_history_files(date,"3");
  }
  
  String path = "/log/";
  path += date;
  path += ".txt";
  File file = SD.open(path, FILE_APPEND);
  if(!file){
      return 1;
  }
  if(!file.print(content.c_str())){
      return 1;
  }
  file.close();
  return 0;
}

String getTime(String NTP,String timeZone){
  Serial.println("Get time");
  Serial.println(NTP);
  long gmtOffset_sec = 3600 * timeZone.toInt();
  configTime(gmtOffset_sec, 0, NTP.c_str());

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    time_t lt;
    lt =time(NULL);
    String date ="";
    date+=String(1900+localtime(&lt)->tm_year).c_str();
    date+=((localtime(&lt)->tm_mon+1)<10?"0":"");
    date+=String(localtime(&lt)->tm_mon+1).c_str();
    date+=((localtime(&lt)->tm_mday)<10?"0":"");
    date+=String(localtime(&lt)->tm_mday).c_str();
    date+=((localtime(&lt)->tm_hour)<10?"0":"");
    date+=String(localtime(&lt)->tm_hour).c_str();
    date+=((localtime(&lt)->tm_min)<10?"0":"");
    date+=String(localtime(&lt)->tm_min).c_str();
    Serial.println(date);
    return date;
  }
  String date ="";
  date+=String(1900+timeinfo.tm_year).c_str();
  date+=((timeinfo.tm_mon+1)<10?"0":"");
  date+=String(timeinfo.tm_mon+1).c_str();
  date+=((timeinfo.tm_mday)<10?"0":"");
  date+=String(timeinfo.tm_mday).c_str();
  date+=((timeinfo.tm_hour)<10?"0":"");
  date+=String(timeinfo.tm_hour).c_str();
  date+=((timeinfo.tm_min)<10?"0":"");
  date+=String(timeinfo.tm_min).c_str();
  Serial.println(date);
  return date;
}

unsigned char DHT_read(float* a, float* b){
  // BUFFER TO RECEIVE
  unsigned char bits[5] = { 0,0,0,0,0 };
  unsigned char cnt = 7;
  unsigned char idx = 0;
  unsigned char sum;
  int DHT_PIN = 27;

  // REQUEST SAMPLE
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(18);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT_PIN, INPUT);

  // ACKNOWLEDGE or TIMEOUT
  unsigned int count = 10000;
  while (digitalRead(DHT_PIN) == LOW)
    if (count-- == 0) return DHT_ERR_TIMEOUT;

  count = 10000;
  while (digitalRead(DHT_PIN) == HIGH)
    if (count-- == 0) return DHT_ERR_TIMEOUT;

  // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
  for (int i = 0; i<40; i++)
  {
    count = 10000;
    while (digitalRead(DHT_PIN) == LOW)
      if (count-- == 0) return DHT_ERR_TIMEOUT;

    unsigned long t = micros();

    count = 10000;
    while (digitalRead(DHT_PIN) == HIGH)
      if (count-- == 0) return DHT_ERR_TIMEOUT;

    if ((micros() - t) > 40) bits[idx] |= (1 << cnt);
    if (cnt == 0)   // next byte?
    {
      cnt = 7;    // restart at MSB
      idx++;      // next byte!
    }
    else cnt--;
  }
  sum = bits[0] + bits[1] + bits[2] + bits[3];
  if (bits[4] != sum) return DHT_ERR_CHECK;

  //==============================================================
  float humidity = (float)((bits[0] << 8) + bits[1]) / 10;
  float temperature = (float)((bits[2] << 8) + bits[3]) / 10;
  //==============================================================
  *a = humidity;
  *b = temperature;

  return DHT_OK;
}

void delete_history_files(String date,String remain){
  File root = SD.open("/log");
  File file = root.openNextFile();
  while(file){
    String filename = file.name();
    int mod=0;
    if(date.substring(4,6).toInt()<remain.toInt()){mod = 88;}
    else{mod =0;}
    if(filename.substring(0,8).toInt()<date.toInt()-(100*(remain.toInt()+mod))){
      String path ="/log/";
      path += filename;
      SD.remove(path);
      }
    file = root.openNextFile();
  }
}

boolean space_use_test(int percentage){
  return ((SD.usedBytes()/SD.totalBytes())*100)>percentage?true:false;
}

void reset_all(){
  write_wifi_eeprom("","", "");
  write_settings_eeprom("0", "pool.ntp.org", "0");
  write_plant_eeprom("","","","","","","");
  ESP.restart();
}
