#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

#define PIXEL_PER_SEGMENT  2     // Number of LEDs in each Segment
#define PIXEL_DIGITS       4     // Number of connected Digits 
#define PIXEL_PIN          2     // GPIO Pin
#define PIXEL_DASH         1     // Binary segment

#define LDR_PIN       A0    // LDR pin
#define DHT_PIN       13    // DHT Sensor pin
#define BUTTON_PIN    12    // Button pin

// Uncomment the type of sensor in use
#define DHT_TYPE    DHT11     // DHT 11
//#define DHT_TYPE    DHT22     // DHT 22 (AM2302)
//#define DHT_TYPE    DHT21     // DHT 21 (AM2301)

#define TIME_FORMAT        12    // 12 = 12 hours format || 24 = 24 hours format 

Adafruit_NeoPixel strip = Adafruit_NeoPixel((PIXEL_PER_SEGMENT * 7 * PIXEL_DIGITS) + (PIXEL_DASH * 2), PIXEL_PIN, NEO_GRB + NEO_KHZ800);
DHT dht(DHT_PIN, DHT_TYPE);

// set Wi-Fi SSID and password
const char *ssid     = "Sayantan";
const char *password = "sayantan";

WiFiUDP ntpUDP;
// 'time.nist.gov' is used (default server) with +1 hour offset (3600 seconds) 60 seconds (60000 milliseconds) update interval
NTPClient timeClient(ntpUDP, "time.nist.gov", 19800, 60000); //GMT+5:30 : 5*3600+30*60=19800

int period = 1000;   //Update frequency
unsigned long time_now = 0;
int Second, Minute, Hour;

// set default brightness
int Brightness = 32;
// current temperature, updated in loop()
int Temperature;

bool Show_Temp = false;

//Digits array
byte digits[12] = {
  //abcdefg
  0b1111110,     // 0
  0b0011000,     // 1
  0b0110111,     // 2
  0b0111101,     // 3
  0b1011001,     // 4
  0b1101101,     // 5
  0b1101111,     // 6
  0b0111000,     // 7
  0b1111111,     // 8
  0b1111101,     // 9
  0b1100110,     // C
  0b1100011,     // F
};

//Clear all the Pixels
void clearDisplay() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show();

  dht.begin();
  pinMode(BUTTON_PIN, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting.");
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");
  timeClient.begin();
  delay(10);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { // check WiFi connection status
    int sensor_val = analogRead(LDR_PIN);
    Brightness = map(sensor_val, 100, 1024, 128, 4);
    timeClient.update();
    int Hours;
    unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time
    Second = second(unix_epoch);                            // get seconds
    Minute = minute(unix_epoch);                            // get minutes
    Hours  = hour(unix_epoch);                              // get hours

    if (TIME_FORMAT == 12) {
      if (Hours > 12) {
        Hour = Hours - 12;
      }
      else
        Hour = Hours;
    }
    else
      Hour = Hours;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    Show_Temp = true;
  }
  else
    Show_Temp = false;

  if (Show_Temp) {
    Temperature = dht.readTemperature();
    Serial.println(Temperature);
    clearDisplay();
    writeDigit(0, Temperature / 10);
    writeDigit(1, Temperature % 10);
    writeDigit(2, 10);
    strip.setPixelColor(28, strip.Color(Brightness, Brightness,  Brightness));
    strip.show();
    delay(3000);
    clearDisplay();
    Show_Temp = false;
  }
  while (millis() > time_now + period) {
    time_now = millis();
    disp_Time();     // Show Time
  }
}

void disp_Time() {
  clearDisplay();
  writeDigit(0, Hour / 10);
  writeDigit(1, Hour % 10);
  writeDigit(2, Minute / 10);
  writeDigit(3, Minute % 10);
  writeDigit(4, Second / 10);
  writeDigit(5, Second % 10);
  disp_Dash();
  strip.show();
}

void disp_Dash() {
  int dot, dash;
  for (int i = 0; i < 2; i++) {
    dot = 2 * (PIXEL_PER_SEGMENT * 7) + i;
    for (int j = 0; j < PIXEL_DASH; j++) {
      dash = dot + j * (2 * (PIXEL_PER_SEGMENT * 7) + 2);
      Second % 2 == 0 ? strip.setPixelColor(dash, strip.Color(Brightness, Brightness,  Brightness)) : strip.setPixelColor(dash, strip.Color(0, 0,  0));
    }
  }
}

void writeDigit(int index, int val) {
  byte digit = digits[val];
  int margin;
  if (index == 0 || index == 1 ) margin = 0;
  if (index == 2 || index == 3 ) margin = 1;
  if (index == 4 || index == 5 ) margin = 2;
  for (int i = 6; i >= 0; i--) {
    int offset = index * (PIXEL_PER_SEGMENT * 7) + i * PIXEL_PER_SEGMENT + margin * 2;
    uint32_t color;
    if (digit & 0x01 != 0) {
      if (index == 0 || index == 1 ) color = strip.Color(Brightness, 0,  0);
      if (index == 2 || index == 3 ) color = strip.Color(0, Brightness,  0);
      if (index == 4 || index == 5 ) color = strip.Color(0,  0, Brightness);
    }
    else
      color = strip.Color(0, 0, 0);

    for (int j = offset; j < offset + PIXEL_PER_SEGMENT; j++) {
      strip.setPixelColor(j, color);
    }
    digit = digit >> 1;
  }
}
