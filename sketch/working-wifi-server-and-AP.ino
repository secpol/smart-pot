/*
To run that code all you need is properly wired ESP.
 
1. Connect your phone to ESP Access Point, credentials are in the code,
2. On your phone browse to:
    192.168.4.1/settings?ssid="local_wifi_ssid"&pass="local_wifi_password" 
    hit Go, wait 5s.
 3. Browse to 192.168.4.1 on your phone to get Local IP.
 4. To reset settings go to either AP IP or local IP and append following /cleareeprom , hit Go or enter.
 
*/
  
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define DEBUG Serial

ESP8266WebServer server(80);

const char* ssid = "smartpot02";
const char* passphrase = "napier01";
String st;
String content;
int statusCode;

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
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      } 
  }
  setupAP();
}

bool testWifi(void) {
  int c = 0;
  DEBUG.println("Wait...");  
  while ( c < 20 ) {
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
      DEBUG.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
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
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
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
  if ( webtype == 1 ) {
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
    server.on("/settings", []() {
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
              EEPROM.write(32+i, qpass[i]);
              DEBUG.print("Wrote: ");
              DEBUG.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"Success\":\"saved to EEPROM... reset\"}";
          statusCode = 200;
          delay(500);
          ESP.restart();
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          DEBUG.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      //IPAddress ip = WiFi.localIP();
      //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      //server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
      //server.on("/", []() {
    IPAddress ip = WiFi.softAPIP();
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
  }
}

void loop() {
  server.handleClient();
}
