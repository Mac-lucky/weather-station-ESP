#include <Adafruit_BME280.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <math.h>


String api_key = "....................";     //  Enter your Write API key from ThingSpeak

#define WIFI_SSID "........................"                             // input your home or public wifi name 
#define WIFI_PASSWORD "........................"                                    //password of wifi ssid

const char* server = "api.thingspeak.com";

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB
#define LOOP_PERIOD 35 // Display updates every 35 ms

uint32_t updateTime = 0;       // time for next update

unsigned long aktualnyCzas = 0;
unsigned long zapamietanyCzas = 0;
unsigned long roznicaCzasu = 0;

int old_analog =  -999; // Value last displayed
int old_digital = -999; // Value last displayed

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = 120, osy = 120; // Saved x & y coords


Adafruit_BME280 bme;
WiFiClient client;



void setup(void) {
  tft.init();
  tft.setRotation(0);
  Serial.begin(115200); // For debug
  tft.fillScreen(TFT_BLACK);
  Serial.begin(115200);
  Wire.begin();         
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                     //try to connect with wifi
  tft.print("Connecting to ");
  tft.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    tft.print(".");
    delay(500);
  }
  tft.println();
  tft.print("Connected to ");
  tft.println(WIFI_SSID);
  tft.print("IP Address is : ");
  tft.println(WiFi.localIP());                                            //print local IP address
  

  analogMeter(); // Draw analogue meter

  updateTime = millis(); // Next update time
  bme.begin(0x76);
  drawTemp();
  drawPressure();
}


void loop() {
    
    float t = bme.readTemperature();
    t = t - 2;
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0F;
    plotNeedle(h, 0);
    writeTemp();
    writePressure();

    aktualnyCzas = millis();
    delay(3000);
    if (client.connect(server,80) & aktualnyCzas - zapamietanyCzas >= 20000UL)   //   "184.106.153.149" or api.thingspeak.com
    {  
      Serial.println("teraz");
      String data_to_send = api_key;
      data_to_send += "&field1=";
      data_to_send += t;
      data_to_send += "&field2=";
      data_to_send += h;
      data_to_send += "&field3=";
      data_to_send += p;

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(data_to_send.length());
      client.print("\n\n");
      client.print(data_to_send);
      client.stop();
      zapamietanyCzas = aktualnyCzas;
    }
}


// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  // Meter outline
  tft.fillRect(0, 0, 239, 126, TFT_GREY);
  tft.fillRect(5, 3, 230, 119, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 120;
    uint16_t y0 = sy * (100 + tl) + 140;
    uint16_t x1 = sx * 100 + 120;
    uint16_t y1 = sy * 100 + 140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120;
    int y2 = sy2 * (100 + tl) + 140;
    int x3 = sx2 * 100 + 120;
    int y3 = sy2 * 100 + 140;

    // Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
    //  tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    //}

    // Green zone limits
    if (i >= 0 && i < 25) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREEN);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREEN);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 120;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120;
    y1 = sy * 100 + 140;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 120;
      y0 = sy * (100 + tl + 10) + 140;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0, y0 - 12, 2); break;
        case -1: tft.drawCentreString("25", x0, y0 - 9, 2); break;
        case 0: tft.drawCentreString("50", x0, y0 - 6, 2); break;
        case 1: tft.drawCentreString("75", x0, y0 - 9, 2); break;
        case 2: tft.drawCentreString("100", x0, y0 - 12, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120;
    y0 = sy * 100 + 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

  tft.drawString("Hum", 5 + 230 - 40, 119 - 20, 2); // Units at bottom right
  tft.drawCentreString("Hum", 120, 70, 3); // Comment out to avoid font 4
  tft.drawRect(5, 3, 230, 119, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; dtostrf(value, 4, 0, buf);
  tft.drawRightString(buf, 40, 119 - 20, 2);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle util new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately id delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("Wilgotno", 120, 70, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_RED);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_MAGENTA);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}

void drawTemp(){
 tft.setTextColor(TFT_WHITE, TFT_BLACK);
 tft.setFreeFont(FSB9);
 tft.drawString("TEMP", 35, 140, GFXFF);
 tft.drawLine(120, 120, 120, 240, TFT_BLUE);
}

void drawPressure(){
 tft.setTextColor(TFT_WHITE, TFT_BLACK);
 tft.setFreeFont(FSB9);
 tft.drawString("CISN", 160, 140, GFXFF);
}

void writeTemp(){
  float t = bme.readTemperature();
  t = t - 2;
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(FSB18);
  tft.setTextPadding(70);
  tft.drawFloat(t, 2, 20, 180, GFXFF);
}

void writePressure(){
  float p = bme.readPressure() / 100.0F;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setFreeFont(FSB18);
  tft.setTextPadding(70);
  tft.drawFloat(p, 1, 140, 180, GFXFF);
}
