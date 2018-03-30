#include <inttypes.h>

#define DHT_OK      1
#define DHT_ERR_CHECK 0
#define DHT_ERR_TIMEOUT -1


unsigned int DHT_PIN = 16;
float humidity;
float temperature;


unsigned char DHT_read()
{
  // BUFFER TO RECEIVE
  unsigned char bits[5] = {0,0,0,0,0};
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
  while(digitalRead(DHT_PIN) == LOW)
    if (count-- == 0) return DHT_ERR_TIMEOUT;

  count = 10000;
  while(digitalRead(DHT_PIN) == HIGH)
    if (count-- == 0) return DHT_ERR_TIMEOUT;

  // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
  for (int i=0; i<40; i++)
  {
    count = 10000;
    while(digitalRead(DHT_PIN) == LOW)
      if (count-- == 0) return DHT_ERR_TIMEOUT;

    unsigned long t = micros();

    count = 10000;
    while(digitalRead(DHT_PIN) == HIGH)
      if (count-- == 0) return DHT_ERR_TIMEOUT;

    if ((micros() - t) > 40) bits[idx] |= (1 << cnt);
    if (cnt == 0)   // next byte?
    {
      cnt = 7;    // restart at MSB
      idx++;      // next byte!
    }
    else cnt--;
  }
  sum = bits[0]+bits[1]+bits[2]+bits[3];
  if(bits[4] != sum) return DHT_ERR_CHECK;

    
//==============================================================
  humidity = (float)((bits[0] << 8)+bits[1])/10;            
  temperature = (float)((bits[2] << 8)+bits[3])/10;
//==============================================================

  return DHT_OK;
}

void setup() {
   pinMode(DHT_PIN,INPUT);
   digitalWrite(DHT_PIN, HIGH);
   Serial.begin(115200);
}
void loop() {
   DHT_read();
   Serial.print("temperature:");
   Serial.println(temperature);
   Serial.print("humidity:");
   Serial.println(humidity);
   delay(500);
}
