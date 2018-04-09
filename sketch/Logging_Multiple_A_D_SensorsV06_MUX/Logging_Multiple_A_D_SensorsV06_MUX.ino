
/* Author: Bartek Zuranski
 * Switching between multiple analog sensors with MUX 4051, within specified time intervals, suma() marks pre-analog read & summarize 16 analog reading for smoothing the output value, 
 * Moisture sensor read should be done after few minutes after powering it up due to stability delay, or only upon powering it up to avoid chance of electrolysis and rusting of the probe.
 * <<<sending uart data over wifi not complete>>>
 * Sensors values are logged to file on SD card
 * Used Sensors:
 * -Digital DHT22 Humidity & Tmp Sensor
 * Analog Sensors:
 * -LDR light sensor, Moisture sensor FC28, Water level sensor ( set of 3 wires) if grounded shows presence of water 
 */


//#include "uartWIFI.h" 
#include <DHT.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <time.h>
#define DHTPIN 5                                             // Pin connected on DHT22
#define DebugOut Serial

DHT dht1(DHTPIN, DHT22);                                     // Defining dht, reads value on pin 5

//WiFi settings
const char* ssid = "Project_Test_Router";                    // replace with your AP ssid */
const char* password = "12345678";                           // replace with your WiFi pass */
const char* host = "pot_01";
float data;                                                  // data to be read from sensor
String text;                                                 // any text
String data2;                                                // string data of the sensor

unsigned long intervalDHT = 1500;                            // Time to wait for DHT reading
unsigned long intervalMST = 2300;                            // Time to wait for MST reading
unsigned long intervalLDR1 = 1733;                           // Time to wait for LDR1 reading
unsigned long intervalWater = 4000;                          // Time to wait for Water lvl check
unsigned long previousMillisDHT = 0;                         // defining sensors time 
unsigned long previousMillisMST = 0; 
unsigned long previousMillisLDR1 = 0;
unsigned long previousMillisWater = 0;
int discharge = 100;                                          // Required delay to read valid sensor data

static bool hasSD = false;
File uploadFile;   // rather not needed ...from example ...
File sensors;

int pinA0 = A0;                                               // Analog Sensors ADC pin read
int ldr_value;                                                // LDR light sensors value                                            
int mst_value;                                                // MST sensors value


//////////////////// WATER LEVEL VARS //////////////////////////////
// storage for Value of MuX Channels:
int a,b,c;  // y0, y1, y2
// bit value of select Mux pins trough 1st, 2nd, 3rd loop
int r[] = { 0, 0, 0 }; 
// selectPins esp to MUX A,B,C
int selPin[] = { 2, 4, 16 };
int count = 0;    // count Mux'Y' pin selection
///////////////////////////////////////////////////////////////////

void setup()
{
 DebugOut.begin(115200);                                        

 for(int pin = 0; pin < 3; pin++){      // setup of Mux select pins
    pinMode(selPin[pin], OUTPUT);
  } 

 // Establish Wifi connection...
 
  DebugOut.begin(115200);
  DebugOut.setDebugOutput(true);
  DebugOut.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.hostname("pot_01");
  WiFi.begin(ssid, password);
  DebugOut.print("Connecting to ");
  DebugOut.println(ssid);

  // Wait for WiFi connection, confirm status
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  DebugOut.println("");
  if(i == 21){
    DebugOut.print("Could not connect to");
    DebugOut.println(ssid);
    while(1) delay(500);
  }else{
  DebugOut.print("Connected! IP address: ");
  DebugOut.println(WiFi.localIP());
  }
  // Set TimneZone in sec, daylightOffset in sec, timeZ servers...
  
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  DebugOut.println("\nWaiting for time");
  while (!time(nullptr)) {
    DebugOut.print(".");
    delay(1000);
  }
  
  // MDNS SERVER SKIPPED //
}// START SD, NO CONF File
void startSD(){  
  if (SD.begin(SS)){                                      // pin SS:15
     DebugOut.println("SD Card initialized.");
     hasSD = true;
  }
}
float LoggingToSD(float data, String text)               // Logs data to SD card
{
  sensors = SD.open("logfile", FILE_WRITE);
  if (sensors) 
  {
    sensors.print(text + " ");
    if(data > 0.1){
    sensors.print(data,1);
    }
    sensors.print(", ");
    //sensors.println(Data);
    sensors.print("Log file size: ");
    sensors.println(sensors.size());                     /* file size for testing purposes */
    sensors.close();
  } 
  else 
  {
    DebugOut.println("No file");
  }
}
//H20+//////////////////////// WATER LVL SENSOR BLOCK////////////////////////////////
String readH2O(){
  // select the read sequence( bit) for 3 channels of water level sensor, 1st read all 0 ...
  for (count=0; count<=2; count++) {
    r[0] = bitRead(count,0);         
    r[1] = bitRead(count,1);         
    r[2] = bitRead(count,2);         
  // truth table / channel selection 
   digitalWrite(selPin[0], r[0]);
   digitalWrite(selPin[1], r[1]);
   digitalWrite(selPin[2], r[2]);
  // store Mux channels y0-2 value (returned sum of 16 readings) No water = no ground & wire = High(16300)
 switch(count){ 
  case 0:
    a = suma();
    if(a < 16300){  
    return String(" Full");
    }
  case 1:
    b = suma();
    if(b < 16300){
    return String(" Half");
    }
  case 2:
    c = suma();
    if(c < 16300){
     // Serial.println("Low");
     return String(" Low");
    }else{
    if(count != 0 && count != 1){
      return String(" EMPTY");
      break;                          // No water Error handling
    }
    }
   }
  }
  DebugOut.println("");
}
//H20-/////////////////////////////////////////////////////////////////////////////////
int suma(){                          // pre read mark & sum for water lvl sensor accuracy, and used for average 
    analogRead(A0);  
    delay(200);
    int sum = 0;
    for (int i = 0; i < 16; i++) {
    sum += analogRead(A0);
    delay(1);
    }
    return sum;
}
//DHT+////////////////////////////////////////////////////////////////////////////////
float tempcDHT22()
{  
  float tempC; 
  tempC = dht1.readTemperature();
  DebugOut.print("Temperature: "); 
  DebugOut.print(tempC,1);
  DebugOut.println("ºC");
  DebugOut.println("");
  return tempC;
}
float tempfDHT22()
{  
  float tempF; 
  tempF = dht1.readTemperature(true);
  DebugOut.print("Temperature: "); 
  DebugOut.print(tempF,1);
  DebugOut.println("ºF");
  DebugOut.println("");
  return tempF;
}
float humidDHT22()
{
  float humid;
  humid = dht1.readHumidity();
  DebugOut.print("Humidity:    "); 
  DebugOut.print(humid); 
  DebugOut.println("%RH");
  DebugOut.println("");
  return humid;
}
//DHT-/////////////////////////////////////////////////////////////////////////////LDR+// 
int loggingLDR1()                              // Analog Light sensor logging: LDR: Phototransistor #1
{   
   //set the selectPins to read channel y4
   digitalWrite(selPin[0],LOW);  // a
   digitalWrite(selPin[1],LOW);  // b
   digitalWrite(selPin[2],HIGH); // c
   analogRead(pinA0);
   delay(100);
   ldr_value = suma()/16;                      // Take average from 16 readings
   ldr_value = map(ldr_value, 0, 1024, 0, 100);// Map analog reading to precentage value 0-100
   //ldr_value = constrain(ldr_value,0,100);   // Calibrate/map reading if necessary
  DebugOut.print("Light Meter: ");             // Measuring light level in precentage of LUX
  DebugOut.print(ldr_value);
  DebugOut.println("% LUX");
  DebugOut.println("");    
  return ldr_value;
}
//LDR-//////////////////////////////////////////////////////////////////////////////MST+//
int loggingMst(){
   //set the selectPins to read channel y3
  digitalWrite(selPin[0],HIGH);  // a
  digitalWrite(selPin[1],HIGH);  // b
  digitalWrite(selPin[2],LOW);   // c
  analogRead(pinA0);
  delay(100);
  mst_value = suma()/16;         // Take average from 16 readings
 // mst_value = mst_value / 4    // TESTING calibration Scale to 8 bits (0 - 255).
  Serial.println(mst_value);     // ...
  mst_value = map(mst_value, 600, 535 ,0, 255);
  mst_value = constrain(mst_value,0,100);
  DebugOut.print("Mositure: ");
  DebugOut.print(mst_value);
  DebugOut.println("%");
  DebugOut.println("");
  return mst_value;
}
//MST-////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  startSD();
  unsigned long currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillisDHT) >= intervalDHT) {
    //pinMode(PIN,OUTPUT);
    //digitalWrite(PIN,HIGH);                  // Optional Power up dht sensor
   // delay(stabilization);
    data = humidDHT22();
    text = "Humidity:      ";
    LoggingToSD(data, text);
    data = tempcDHT22();
    text = "TemperatureºC: ";
    LoggingToSD(data, text);
    data = tempfDHT22();
    text = "TemperatureºF: ";
    LoggingToSD(data, text);
    previousMillisDHT = currentMillis;      // Save current time dht pin previousMillis
    //digitalWrite(dht,LOW);
    }    
  if ((unsigned long)(currentMillis - previousMillisMST) >= intervalMST) {
   data = loggingMst();
   text = "Mositure:      ";
   LoggingToSD(data, text);
   //delay(discharge);
   previousMillisMST = currentMillis;      
  }
   if ((unsigned long)(currentMillis - previousMillisLDR1) >= intervalLDR1) {
    data = loggingLDR1();
    text = "Light % LUX:   ";
    LoggingToSD(data, text);
    //delay(discharge);
    previousMillisLDR1 = currentMillis;     
  }
  if ((unsigned long)(currentMillis - previousMillisWater) >= intervalWater) {
    DebugOut.print("Water Level:   ");
    DebugOut.println(readH2O());
    DebugOut.println("");
    data = 0;
    data2 = readH2O();
    text = "Water Level:   " + data2;
    LoggingToSD(data, text);
    //delay(discharge);
    previousMillisWater = currentMillis; 
    }
 }
 

