#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include <SoftwareSerial.h>

// These are 'flexible' lines that can be changed
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8 // RST can be set to -1 if you tie it to Arduino's reset
SoftwareSerial mySerial(3,5);
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // put your setup code here, to run once:
tft.begin();
tft.setRotation(1);
tft.fillScreen(HX8357_BLACK);
tft.setCursor (0,0);
tft.setTextColor(HX8357_YELLOW,HX8357_BLACK);
tft.setTextSize(2);
tft.print("F14 TOMCAT");
mySerial.begin(38400);
}

void loop() {
  byte byte_read = 0;
  byte byte_available = 0;
if (mySerial.available()) {
  byte_read = mySerial.read();
  byte_available = 1;
}
if (byte_available) {
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
      
  }
  byte_available = 0;
  tft.setCursor (0,30);
  tft.print("Last byte received ");
  tft.print(byte_read);
  tft.print("    ");
}

}
