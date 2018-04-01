/**
 * Need to add lib from arduinoIDE:
 * ArduinoJson
 * 
 * SDCard shield can only 
 * read file which filename extension 
 * is under 4 letter, so "*.json" cannot
 * use in this program.
 * 
*/

#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>

#define BAUD_RATE 115200

/**
 * 打开文件并返回文件内容
 * Open the file and return the content of the file
*/
String readFile(String path){
  File myFile;
  String fileBuffer;
  Serial.begin(BAUD_RATE);
  if (!SD.begin(4)) {
    Serial.println("SDCard ERROR");
    return "-";}
  myFile = SD.open(path);
  if (myFile) 
  {
    while(myFile.available()){fileBuffer += char(myFile.read());}
    myFile.close();
  } 
  else {return "--";}
  return fileBuffer;
}


void setup() {
  Serial.begin(BAUD_RATE);

  /**
   * 打开
   * OPEN File
  */
  DynamicJsonBuffer jsonBuffer;
  Serial.println(readFile("test.txt"));//DEBUG
  String input = readFile("test.txt");
  JsonObject& root = jsonBuffer.parseObject(input);
  /**
   * 读取数据
   * READ DATA
  */
  int intType = root[String("intType")];
  long longType = root[String("longType")];
  String stringType = root["stringType"];
  /**
   * 输出
   * OUTPUT
  */
  Serial.println(longType);
  Serial.println(stringType);
  Serial.println(intType);
}

void loop(){
  delay(1000);
}
