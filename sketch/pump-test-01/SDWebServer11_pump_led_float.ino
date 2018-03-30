
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <time.h>
#define DBG_OUTPUT_PORT Serial


const char* ssid = "....."; /* replace with your AP ssid */
const char* password = "....."; /* replace with your WiFi pass */
const char* host = "smartpot";

DHT dhtsens(4, DHT22);
ESP8266WebServer server(80);
int sensorsPin = 0;
float adcValue = analogRead(A0);
//pinMode(pno, OUTPUT);

unsigned long intervaldhttmp = 2000; /* the time we need to wait for sensor reading msec */
unsigned long intervaldhthum = 2000; /* the time we need to wait for sensor reading msec */
unsigned long intervaltime = 10000; /* the time we need to wait for logfile timestamp msec */
unsigned long intervaltemp = 2000;
unsigned long previousMillisdhttmp = 0; /* millis() returns an unsigned long */
unsigned long previousMillisdhthum = 0; /* millis() returns an unsigned long */
unsigned long previousMillistime = 0; /* millis() returns an unsigned long */
unsigned long previousMillistemp = 0;

static bool hasSD = false;
File uploadFile;
//File dht;
File sensors;
//int pno = 2;

void setup(void){
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.hostname("smartpot");
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  
  /* set as follow: (timezone in sec, daylightOffset in sec, 
  server_name1, server_name2, server_name3) */
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println("");
  if(i == 21){
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while(1) delay(500);
  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }


  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/edit", HTTP_POST, [](){ returnOK(); }, handleFileUpload);
  server.onNotFound(handleNotFound);

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

  if (SD.begin(SS)){
     DBG_OUTPUT_PORT.println("SD Card initialized.");
     hasSD = true;
  }
}
/**/

void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSdCard(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SD.open(path.c_str());
  if(dataFile.isDirectory()){
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (server.hasArg("download")) dataType = "application/octet-stream";

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    if(SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: "); DBG_OUTPUT_PORT.println(upload.filename);
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    DBG_OUTPUT_PORT.print("Upload: WRITE, Bytes: "); DBG_OUTPUT_PORT.println(upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(uploadFile) uploadFile.close();
    DBG_OUTPUT_PORT.print("Upload: END, Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void deleteRecursive(String path){
  File file = SD.open((char *)path.c_str());
  if(!file.isDirectory()){
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while(true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" +entry.name();
    if(entry.isDirectory()){
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if(path.indexOf('.') > 0){
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if(file){
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void printDirectory() {
  if(!server.hasArg("dir")) return returnFail("BAD ARGS");
  String path = server.arg("dir");
  if(path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if(!dir.isDirectory()){
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();

  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
    break;

    String output;
    if (cnt > 0)
      output = ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
 }
 server.sendContent("]");
 dir.close();
}

void handleNotFound(){
  if(hasSD && loadFromSdCard(server.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  DBG_OUTPUT_PORT.print(message);
}

float tempDHT22()
{  
  float temp; 
  temp = dhtsens.readTemperature();

  Serial.print("Digital temperature: "); 
  Serial.print(temp,1);
  Serial.println("ºC");
  Serial.println("");

  return temp;
}

float humidDHT22()
{
  float humid;
  humid = dhtsens.readHumidity();

  Serial.print("Digital humidity:    "); 
  Serial.print(humid); 
  Serial.println("%RH");
  Serial.println("");

  return humid;
}

float LoggingToSD(float data, String text)
{
  sensors = SD.open("logfile", FILE_WRITE);
  if (sensors) 
  {
    sensors.print(text + " ");
    sensors.print(data);
    sensors.print(", ");
    //sensors.println(Data);
    sensors.print("Log file size: ");
    sensors.println(sensors.size()); /* file size for testing purposes */
    sensors.close();
  } 
  else 
  {
    Serial.println("No file");
  }
}

float AnalogRead()
{
  int reading = analogRead(sensorsPin);
  float voltage = reading * 3.3; 
  voltage /= 1024.0;
  float temperatureC = (voltage - 0.5) * 10;
  Serial.print("Analog temperature: ");
  Serial.println(temperatureC);
  if (temperatureC >= 21.00) {
    return 1;
  }
  else {
    return 0;
  }
}

void loop(void){
  server.handleClient();
  pinMode(5, OUTPUT);
  float data;
  String text;
  //float DHT22_temp = dhtsens.readTemperature();
  
  /* get current time stamp */ 
  /* only need one for both if-statements */ 
  unsigned long currentMillis = millis();

  /* interval to read current time and print in console */
  if ((unsigned long)(currentMillis - previousMillistime) >= intervaltime) {
    time_t now = time(nullptr);
    Serial.print(ctime(&now));
    Serial.println("");
    Serial.println("");
    previousMillistime = currentMillis;
  }
    /* interval to read and save DHT sensor data (temperature) to file */ 
  if ((unsigned long)(currentMillis - previousMillisdhttmp) >= intervaldhttmp) {
    data = tempDHT22();
    text = "Temperature: ";
    LoggingToSD(data, text);
    previousMillisdhttmp = currentMillis;
  }
      /* interval to read and save DHT sensor data (humidity) to file */ 
  if ((unsigned long)(currentMillis - previousMillisdhthum) >= intervaldhthum) {
    data = humidDHT22();
    text = "Humidity: ";
    LoggingToSD(data, text);
    previousMillisdhthum = currentMillis;
  }
  
  if ((unsigned long)(currentMillis - previousMillistemp) >= intervaltemp) {
    if (AnalogRead()) 
    {
      digitalWrite(5, HIGH);
      delay(500);
      digitalWrite(5, LOW);
      delay(2000);
      Serial.print("Pump active");
      Serial.println("");
      Serial.println("");
      
    } 
    else
    {
      digitalWrite(5, LOW);
      Serial.print("Pump inactive"); 
      Serial.println("");
      Serial.println("");
    }
    previousMillistemp = currentMillis;
  }
  
}


