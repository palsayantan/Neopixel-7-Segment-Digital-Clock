#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <RTClib.h>

#define PIXEL_PER_SEGMENT  2     // Number of LEDs in each Segment
#define PIXEL_DIGITS       4     // Number of connected Digits 
#define PIXEL_PIN          2     // GPIO Pin
#define PIXEL_DASH         1     // Binary segment

#define LDR_PIN       A0    // LDR pin
#define BUTTON_PIN    12    // Button pin

#define TIME_FORMAT        12    // 12 = 12 hours format || 24 = 24 hours format 

Adafruit_NeoPixel strip = Adafruit_NeoPixel((PIXEL_PER_SEGMENT * 7 * PIXEL_DIGITS) + (PIXEL_DASH * 2), PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// set Wi-Fi SSID and password
const char *ssid     = "Sayantan";
const char *password = "SayantaN";

RTC_DS3231 rtc;

bool Set_Time = false;
bool night_mode = false;
int brightness_threshold = 50;

WiFiUDP ntpUDP;
// 'time.nist.gov' is used (default server) with +1 hour offset (3600 seconds) 60 seconds (60000 milliseconds) update interval
NTPClient timeClient(ntpUDP, "time.nist.gov", 19800, 60000); //GMT+5:30 : 5*3600+30*60=19800

int period = 30000;     // Color changing interval
unsigned long time_now = -30000;
int Second, Minute, Hour;

// set default brightness
uint32_t r = 180, g = 0, b = 0;

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
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ISR, RISING);
  WiFi.begin(ssid, password);
  Serial.print("Connecting...");
  delay(1000);

  if ( WiFi.status() == WL_CONNECTED ) {
    Serial.println("connected");
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(60, 60, 60));
    }
    strip.show();
    Set_Time = true;
  }
  else
    Set_Time = false;
}
/*
  ICACHE_RAM_ATTR void ISR() {
  Set_Time = true;
  }
*/
void loop() {
  if (Set_Time) {
    timeClient.begin();

    // check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.update();
      unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time
      Second = second(unix_epoch);                            // get seconds
      Minute = minute(unix_epoch);                            // get minutes
      Hour  = hour(unix_epoch);                              // get hours

      if (TIME_FORMAT == 12) {
        if (Hour >= 13) {
          Hour = Hour - 12;
        }
        if (Hour == 00) {
          Hour = 12;
        }
        else
          Hour = Hour;
      }
      else if (TIME_FORMAT == 24) {
        Hour = Hour;
      }
      Serial.print(Hour);
      Serial.print(":");
      Serial.print(Minute);
      Serial.print(":");
      Serial.print(Second);
      Serial.println(" ");
      rtc.adjust(DateTime(2020, 1, 1, Hour, Minute, Second));
      Serial.println("Time Set");
      WiFi.disconnect();
      Set_Time = false;
    }
  }
  else {
    DateTime now = rtc.now();

    Second = now.second();
    Minute = now.minute();
    Hour  = now.hour();

    if (Hour >= 13) {
      Hour = Hour - 12;
    }
    if (Hour == 00) {
      Hour = 12;
    }
    else
      Hour = Hour;

    uint32_t sensor_val = analogRead(LDR_PIN);
    if (sensor_val < brightness_threshold) {
      r = 6;
      g = 6;
      b = 6;
      night_mode = true;
      time_now = -30000;
    }
    else if (sensor_val > brightness_threshold && night_mode) {
      ESP.restart();
    }
    if (!night_mode) {
      disp_Dash();
    }
    while (millis() > time_now + period) {
      time_now = millis();
      Serial.print("Time Now : ");
      Serial.print(Hour);
      Serial.print(":");
      Serial.print(Minute);
      Serial.print(":");
      Serial.print(Second);
      Serial.println(" ");
      disp_Time();     // Show Time
    }
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
  strip.show();
}

void disp_Dash() {
  int dot, dash;
  for (int i = 0; i < 2; i++) {
    dot = 2 * (PIXEL_PER_SEGMENT * 7) + i;
    for (int j = 0; j < PIXEL_DASH; j++) {
      dash = dot + j * (2 * (PIXEL_PER_SEGMENT * 7) + 2);
      Second % 2 == 0 ? strip.setPixelColor(dash, strip.Color(r / 3, g / 3,  b / 3)) : strip.setPixelColor(dash, strip.Color(0, 0,  0));
    }
  }
  strip.show();
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
      if (!night_mode) {
        if (r > 0 && b == 0) {
          r--;
          g++;
        }
        if (g > 0 && r == 0) {
          g--;
          b++;
        }
        if (b > 0 && g == 0) {
          r++;
          b--;
        }
      }
      color = strip.Color(r / 3, g / 3, b / 3);
    }
    else
      color = strip.Color(0, 0, 0);

    for (int j = offset; j < offset + PIXEL_PER_SEGMENT; j++) {
      strip.setPixelColor(j, color);
    }
    digit = digit >> 1;
  }
}
