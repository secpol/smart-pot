#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <DNSServer.h>
//#include "WiFiManager.h"
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <time.h>

#define DEBUG Serial //other name for serial
const char* ssid = "......"; //your AP ssid
const char* password = "......"; //your AP pass
const char* serverIndex = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"; //form that show up when browsing website
static bool hasSD = false;

ESP8266WebServer server(80);
File uploadFile;

void handleFileUpload(){
  if(server.uri() != "/upload") return; //if not equal to
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    if(SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DEBUG.print("Upload: START, filename: "); 
    DEBUG.println(upload.filename);
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    DEBUG.print("Upload: WRITE, Bytes: "); 
    DEBUG.println(upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(uploadFile) uploadFile.close();
    DEBUG.print("Upload: END, Size: "); 
    DEBUG.println(upload.totalSize);
  }
}

void setup(void)
{
  DEBUG.begin(115200);
  
  if (SD.begin(SS)){
     DEBUG.println("SD Card initialized.");
     hasSD = true;
  }

  DEBUG.setDebugOutput(true);
  DEBUG.print("\n");
  DEBUG.println("Welcome to WebServer File System");
  DEBUG.println("");
  WiFi.mode(WIFI_AP_STA); //start AP and client mode simultaneously
  WiFi.begin(ssid, password); 
  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {
    DEBUG.print("Local IP: ");
    DEBUG.println(WiFi.localIP());
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", serverIndex);
    });
    //server.on("/reset", HTTP_GET, reset);
    server.onFileUpload(handleFileUpload);
    server.on("/upload", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", "OK");
    });
    server.begin();
  }
  
}

void loop(void) {
  server.handleClient();
}
