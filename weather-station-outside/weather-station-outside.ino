#include <OneWire.h>
#include <DS18B20.h>
#include <BH1750.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <math.h>


String api_key = "....................";     //  Enter your Write API key from ThingSpeak

#define WIFI_SSID "........................"                             // input your home or public wifi name 
#define WIFI_PASSWORD "........................"                                    //password of wifi ssid
#define WindSensorPin 14                                             // The pin location of the anemometer sensor
#define ONE_WIRE_BUS 12

const char* server = "api.thingspeak.com";

volatile unsigned long Rotations; // cup rotation counter used in interrupt routine
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine

float windSpeed; // speed miles per hour
float windDirection;

const byte windDirPin = A0;

BH1750 lightMeter;
Adafruit_BME280 bme;
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);
WiFiClient client;



//////////////// Functions //////////////////////////////////////////

// This is the function that the interrupt calls to increment the rotation count
ICACHE_RAM_ATTR void isr_rotation () {

if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact.
Rotations++;
ContactBounceTime = millis();
}
}


void windSpeedandDir() {
  Rotations = 0; // Set Rotations count to 0 ready for calculations
  
  delay (8000); // Wait 8 second to average

  windSpeed = Rotations * 0.3;  //one rotation every second is 2.4 km/h, so aveerage between 8 sec i 2.4/8
  Serial.print("Rotations: ");
  Serial.println(Rotations);
  // get wind direction
  float dirpin = analogRead(windDirPin)*(3.3 / 1023.0);

  
  if(dirpin > 2.60 &&  dirpin < 2.70 ){
    windDirection = 270;
  }
  if(dirpin > 1.50 &&  dirpin < 1.70 ){
    windDirection = 315;
  }
  if(dirpin > 0.20 &&  dirpin < 0.40 ){
    windDirection = 0;
  }
  if(dirpin > 0.50 &&  dirpin < 0.70 ){
    windDirection = 45;
  }
  if(dirpin > 0.80 &&  dirpin < 1.10 ){
    windDirection = 90;
  }
  if(dirpin > 2.00 &&  dirpin < 2.20 ){
    windDirection = 135;
  }
  if(dirpin > 3.05 &&  dirpin < 3.25 ){
    windDirection = 180;
  }
  if(dirpin > 2.95 &&  dirpin < 3.05 ){
    windDirection = 225;
  }    


  Serial.print("Windspeed: ");
  Serial.print(windSpeed);
  Serial.println(" km/h ");
  Serial.print("Wind Direction: ");
  Serial.println(windDirection);
}
 

void setup() 
{
  Serial.begin(115200);
  delay(1000);       
  Wire.begin();
  WiFi.forceSleepWake();         
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                     //try to connect with wifi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP());                                            //print local IP address
  
  pinMode(WindSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING);
  pinMode(windDirPin, INPUT);
  
  sensor.begin();
  bme.begin(0x76);                                                          //Start reading dht sensor
  lightMeter.begin();
}

void loop() {
  windSpeedandDir();
  float lux = lightMeter.readLightLevel();   
  if (lux < 1){
    lux = 0;
  }

  sensor.requestTemperatures();
  while (!sensor.isConversionComplete());  // wait until sensor is ready
  
  float t = sensor.getTempC();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F;
    
  if (isnan(h) || isnan(t) || isnan(p)) {                                                // Check if any reads failed and exit early (to try again).
    Serial.println(F("Failed to read from BME280 sensor!"));
    return;
  }

    if (isnan(lux)) {                                                
    Serial.println(F("Failed to read from BH1750"));
    return;
  }

    if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
    {  
      String data_to_send = api_key;
      data_to_send += "&field1=";
      data_to_send += h;
      data_to_send += "&field2=";
      data_to_send += t;
      data_to_send += "&field3=";
      data_to_send += p;
      data_to_send += "&field4=";
      data_to_send += lux;
      data_to_send += "&field5=";
      data_to_send += windSpeed;
      data_to_send += "&field6=";
      data_to_send += windDirection;
      data_to_send += "\r\n\r\n";
      
      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(data_to_send.length());
      client.print("\n\n");
      client.print(data_to_send);
      delay(1000);
      Serial.print("Temperature: ");
      Serial.println(t);
      Serial.print("Humidity: ");
      Serial.println(h);
      Serial.print("Light: ");
      Serial.print(lux);
      Serial.println(" lx");
      Serial.println("Send to Thingspeak.");
    }
client.stop();

Serial.print("Waiting...");
  // delay(8000);
  // thingspeak needs minimum 15 sec delay between updates, i've set it to 60 seconds
  ESP.deepSleep(45e6);
}
