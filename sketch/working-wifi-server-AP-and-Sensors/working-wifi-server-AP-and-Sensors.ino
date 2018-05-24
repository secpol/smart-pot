/*
To run that code all you need is properly wired ESP.

1. Connect your phone to ESP Access Point, credentials are in the code,
2. On your phone browse to:
x.x.x.x/wifiSetup?ssid=napier5gee&pass=fourwordsalluppercase (where ssid and pass are your home access point credentials)
hit Go, wait 5s.
3. Browse to 192.168.4.1 on your phone to get Local IP, access to server using provided IP.
4. To reset settings browse either AP IP or local IP and append following to IP address: /cleareeprom , hit Go or enter.

*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>

#define DEBUG Serial
#define DBG_OUTPUT_PORT Serial

ESP8266WebServer server(80);

const char* ssid = "smartpot01";
const char* passphrase = "napier01";
static bool hasSD = false;
String st;
String content;
int statusCode;
File conf;



String readFile(String path) {
  File myFile;
  String fileBuffer;
  if (!SD.begin(4)) {
    Serial.println("SDCard ERROR");
    return "-";
  }
  myFile = SD.open(path);
  if (myFile)
  {
    while (myFile.available()) { fileBuffer += char(myFile.read()); }
    myFile.close();
  }
  else { return "--"; }
  return fileBuffer;
}


int writeFile(String path, String sensorsData) {
  File myFile;
  myFile = SD.open(path, FILE_WRITE);
  if (myFile) {
    Serial.print("1 row data write");
    myFile.println(sensorsData);
    myFile.close();
    return 0;
  }
  else {
    return 1;
  }
}

int deleteFile() {

}

int deleteHistory() {

}











//Sensors' Pins & Marco
int pin0 = 5;
int pin1 = 4;
int pin2 = 16;
int DHT_PIN = 16;
#define DHT_OK      1
#define DHT_ERR_CHECK 0
#define DHT_ERR_TIMEOUT -1


void setup() {
  DEBUG.begin(115200);
  EEPROM.begin(512);
  delay(10);
  DEBUG.println();
  DEBUG.println();
  DEBUG.println("Startup");
  // read eeprom for ssid and pass
  DEBUG.println("Reading EEPROM SSID");
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  DEBUG.print("SSID: ");
  DEBUG.println(esid);
  DEBUG.println("Reading EEPROM password");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  DEBUG.print("PASS: ");
  DEBUG.println(epass);
  if (esid.length() > 1) {
    WiFi.begin();
    if (testWifi()) {
      launchWeb(0);
      return;
    }
  }
  if (SD.begin(SS)) {
    DBG_OUTPUT_PORT.println("SD Card initialized.");
    hasSD = true;
  }
  setupAP();
}

bool testWifi(void) {
  int c = 0;
  DEBUG.println("Wait...");
  while (c < 20) {
    if (WiFi.status() == WL_CONNECTED) { return true; }
    delay(500);
    DEBUG.print(WiFi.status());
    c++;
  }
  DEBUG.println("");
  DEBUG.println("Connection timed out, AP mode");
  return false;
}

void launchWeb(int webtype) {
  DEBUG.println("");
  DEBUG.println("WiFi connected");
  DEBUG.print("Local IP: ");
  DEBUG.println(WiFi.localIP());
  DEBUG.print("SoftAP IP: ");
  DEBUG.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  DEBUG.println("Server started");
}

void setupAP(void) {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  DEBUG.println("scan done");
  if (n == 0)
    DEBUG.println("no networks found");
  else
  {
    DEBUG.print(n);
    DEBUG.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      DEBUG.print(i + 1);
      DEBUG.print(": ");
      DEBUG.print(WiFi.SSID(i));
      DEBUG.print(" (");
      DEBUG.print(WiFi.RSSI(i));
      DEBUG.print(")");
      DEBUG.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  DEBUG.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);

  DEBUG.println("softap");
  launchWeb(1);
  DEBUG.println("over");
}

void createWebServer(int webtype)
{
  if (webtype == 1) {

    /* Main webpage of the server, displays details about device when device is in AP mode */

    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String page = "{";
      page += ("\"Soft AP IP\":\[\"");
      page += WiFi.softAPIP().toString();
      page += ("\"\]\,\n\n");
      page += ("\"Chip ID\":\[\"");
      page += ESP.getChipId();
      page += ("\"\]\,\n\n");
      page += ("\"Flash Chip ID\":\[\"");
      page += ESP.getFlashChipId();
      page += ("\"\]\,\n\n");
      page += ("\"IDE Flash Size\":\[\"");
      page += ESP.getFlashChipSize();
      page += ("\"\]\,\n\n");
      page += ("\"Real Flash Size\":\[\"");
      page += ESP.getFlashChipRealSize();
      page += ("\"\]\,\n\n");
      page += ("\"Soft AP MAC\":\[\"");
      page += WiFi.softAPmacAddress();
      page += ("\"\]\,\n\n");
      page += ("\"Station MAC\":\[\"");
      page += WiFi.macAddress();
      page += ("\"]}");
      server.send(200, "application/json", page);
    });

    /* This URL is used to take two specific arguments: SSID and password and store them in EEPROM of the device,
    usage e.g.: x.x.x.x/wifiSetup?ssid=napier5gee&pass=fourwordsalluppercase */

    server.on("/wifiSetup", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        DEBUG.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        DEBUG.println(qsid);
        DEBUG.println("");
        DEBUG.println(qpass);
        DEBUG.println("");

        DEBUG.println("Writing EEPROM SSID:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          DEBUG.print("Wrote: ");
          DEBUG.println(qsid[i]);
        }
        DEBUG.println("Writing EEPROM password:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          DEBUG.print("Wrote: ");
          DEBUG.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to EEPROM... reset\"}";
        statusCode = 200;
        delay(500);
        ESP.restart();
      }
      else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        DEBUG.println("Sending 404");
      }
      server.send(statusCode, "application/json", content);
    });
  }
  else if (webtype == 0) {

    /* Main webpage of the server, displays server Local IP */

    server.on("/", []() {
      String page = "{";
      page += ("\"Server IP\":\[\"");
      page += WiFi.localIP().toString();
      page += ("\"]}");
      server.send(200, "application/json", page);
    });

    /* This URL is used to display details about the hardware, can be used to distinguish from 2 or more different pots */

    server.on("/details", []() {
      String page = "{";
      page += ("\"Soft AP IP\":\[\"");
      page += WiFi.softAPIP().toString();
      page += ("\"\]\,\n\n");
      page += ("\"Server IP\":\[\"");
      page += WiFi.localIP().toString();
      page += ("\"\]\,\n\n");
      page += ("\"Chip ID\":\[\"");
      page += ESP.getChipId();
      page += ("\"\]\,\n\n");
      page += ("\"Flash Chip ID\":\[\"");
      page += ESP.getFlashChipId();
      page += ("\"\]\,\n\n");
      page += ("\"IDE Flash Size\":\[\"");
      page += ESP.getFlashChipSize();
      page += ("\"\]\,\n\n");
      page += ("\"Real Flash Size\":\[\"");
      page += ESP.getFlashChipRealSize();
      page += ("\"\]\,\n\n");
      page += ("\"Soft AP MAC\":\[\"");
      page += WiFi.softAPmacAddress();
      page += ("\"\]\,\n\n");
      page += ("\"Station MAC\":\[\"");
      page += WiFi.macAddress();
      page += ("\"]}");
      server.send(200, "application/json", page);
    });

    /* This URL is used to take any number of arguments and save them to the file,
    usage e.g.: x.x.x.x/uploadSettings?arg1=1000&arg2=2000 */

    server.on("/uploadSettings", []() {
      String message = "";
      for (int i = 0; i < server.args(); i++) {
        message += server.arg(i) + "\n";          //Get the value of the parameter
      }
      conf = SD.open("config.ini", O_WRITE);
      if (conf)
      {
        conf.print(message);
        conf.close();
        server.send(200, "text/plain", message + "Data saved to file.");
      }
      else
      {
        DEBUG.println("No file");
        server.send(200, "text/plain", message + "File corrupt or not exists, data not delivered.");
      }
    });

    /* This URL is used to clear SSID and password from EEPROM memory of the device */

    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      DEBUG.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
      WiFi.disconnect();
      delay(500);
      ESP.restart();
    });

    /*传感器参数发送*/
    server.on("/log", []() {
      String message = "{\n\"data\": [";
      String logData = "";
      if (server.argName(0) == "date") {
        String filePath = "/log/";
        filePath += server.arg(0);
        filePath += ".txt";
        logData += readFile(filePath);
        if (logData == "-" || logData == "--") {
          server.send(404, "text/plain", "Not Found!");
        }
        else {
          message += logData;
          message += "]}";
          server.send(200, "application/json", message);
        }
      }
      else {
        server.send(404, "text/plain", "Not Found!");
      }
    });

  }
}

void loop() {
  server.handleClient();
}




/**
* Returns a string containing JSON of all kinds of sensor data.
*/
String sensorData() {
  String datalog = "\"power\":";
  int power = 0;
  int waterLevel = 0;
  float temperature = 0;
  int soil = 0;
  float humidity = 0;
  int light = 0;

  pinMode(pin0, OUTPUT);
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  digitalWrite(pin0, LOW);
  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);

  power = readMux(0);
  datalog += ("\"" + String(power) + "\",\"soil\":");
  soil = readMux(1);
  datalog += ("\"" + String(soil) + "\",\"light\":");
  light = readMux(2);
  datalog += ("\"" + String(light) + "\",\"waterlevel\":");
  for (int i = 3; i<8; i++) {
    waterLevel += (readMux(i)>512) ? 1 : 0;
  }
  datalog += ("\"" + String(waterLevel) + "\",\"temperature\":");
  delay(100);
  while (DHT_read(&humidity, &temperature) != DHT_OK) { delay(100); }
  datalog += ("\"" + String(temperature) + "\",\"humidity\":");
  datalog += ("\"" + String(humidity) + "\"");

  //CHANGE THE TIME WHEN SLEEPING!
//  if(Hours.toInt()==23 && Minutes.toInt()==55){
//    datalog+="\n";
//    }
//  else{ datalog+=",\n"; }
  return datalog;
}

/**
* 返回读取温湿度是否成功，将湿度和温度数据赋值给变量a,b
*  Return the state of the reading operation.
*  The temperature is assigned to the first parameter, and the humidity is assigned to second parameters.
*
*  This function is called by sensorData ()
*
*/
unsigned char DHT_read(float* a, float* b)
{
  // BUFFER TO RECEIVE
  unsigned char bits[5] = { 0,0,0,0,0 };
  unsigned char cnt = 7;
  unsigned char idx = 0;
  unsigned char sum;


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

int readMux(int channel) {
  int controlPin[] = { pin0, pin1, pin2 };
  int muxChannel[8][3] = {
    { 0, 0, 0 },
    { 1, 0, 0 }, //channel 1
    { 0, 1, 0 }, //channel 2 
    { 1, 1, 0 }, //channel 3
    { 0, 0, 1 }, //channel 4
    { 1, 0, 1 }, //channel 5
    { 0, 1, 1 }, //channel 6
    { 1, 1, 1 }, //channel 7
  };
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }
  delay(1000);
  return analogRead(A0);
}
