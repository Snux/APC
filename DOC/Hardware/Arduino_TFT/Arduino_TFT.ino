#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include <SoftwareSerial.h>

// These are 'flexible' lines that can be changed
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8 // RST can be set to -1 if you tie it to Arduino's reset
SoftwareSerial mySerial(3,5); // pin 3 RX, pin 5 TX
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // put your setup code here, to run once:
tft.begin();
tft.setRotation(2);
tft.fillScreen(HX8357_BLACK);
tft.setCursor (0,0);
tft.setTextColor(HX8357_YELLOW,HX8357_BLACK);
tft.setTextSize(2);
tft.print("F14 TOMCAT Second Sortie");
tft.setTextSize(1);
mySerial.begin(38400);
}

void loop() {
  byte byte_read = 0;
  byte byte_available = 0;
  byte i = 0;
if (mySerial.available()) {
  byte_read = mySerial.read();
  byte_available = 1;
}
if (byte_available) {
  tft.setTextColor(HX8357_YELLOW,HX8357_BLACK);
  switch (byte_read) {
    case 64:
      tft.setCursor(0,50);
      tft.print("LEDsetColorMode 0");
      break;
    case 65:
      tft.setCursor(0,50);
      tft.print("LEDsetColorMode 1");
      break;
    case 66:
      tft.setCursor(0,50);
      tft.print("LEDsetColorMode 2");
      break;
    case 67:
      tft.setCursor(0,50);
      tft.print("LEDsetColorMode 3");
      break;
    case 68:
      tft.setCursor(0,50);
      tft.print("LEDsetColorMode 4");
      break;
    case 100:
      tft.setCursor(0,70);
      tft.print("Radar ring is ON    ");
      break;
    case 101:
      tft.setCursor(0,70);
      tft.print("Radar ring is OFF    ");
      break;
    case 200:
      i = mySerial.read();
      tft.setCursor(0,90);
      tft.print("Left lock status   - ");
      tft.setTextColor(HX8357_RED,HX8357_BLACK);
      switch (i) {
        case 200:
          tft.print("NOT LIT    ");
          break;
        case 201:
          tft.print("LIT        ");
          break;
        case 202:
          tft.print("BALL LOCKED");
          break;
      }
      i = mySerial.read();
      tft.setCursor(0,100);
      tft.setTextColor(HX8357_YELLOW,HX8357_BLACK);
      tft.print("Centre lock status - ");
      tft.setTextColor(HX8357_RED,HX8357_BLACK);
      switch (i) {
        case 200:
          tft.print("NOT LIT    ");
          break;
        case 201:
          tft.print("LIT        ");
          break;
        case 202:
          tft.print("BALL LOCKED");
          break;
      }
      i = mySerial.read();
      tft.setCursor(0,110);
      tft.setTextColor(HX8357_YELLOW,HX8357_BLACK);
      tft.print("Right lock status  - ");
      tft.setTextColor(HX8357_RED,HX8357_BLACK);
      switch (i) {
        case 200:
          tft.print("NOT LIT    ");
          break;
        case 201:
          tft.print("LIT        ");
          break;
        case 202:
          tft.print("BALL LOCKED");
          break;
      }


  }
  byte_available = 0;
  tft.setCursor (0,30);
  tft.setTextColor(HX8357_YELLOW,HX8357_BLACK);
  tft.print("Last byte received from APC_LED board ");
  tft.print(byte_read);
  tft.print("    ");
}

}
