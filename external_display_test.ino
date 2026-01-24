// External 3.5" ILI9488 Display Test for Wio Terminal
// Corrected pin mapping

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Wio Terminal back header pin mapping:
// Pin 13 = D2, Pin 15 = D3, Pin 24 = D8
#define TFT_CS   8   // Pin 24 (SS/D8)
#define TFT_DC   3   // Pin 15 (D3)
#define TFT_RST  2   // Pin 13 (D2)

// Use hardware SPI (MOSI=Pin19, SCK=Pin23)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Display Test");

  tft.begin();
  Serial.println("begin done");

  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  Serial.println("fillScreen done");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(30, 50);
  tft.println("HELLO!");

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(30, 100);
  tft.println("Display Working!");

  tft.drawRect(10, 10, 300, 220, ILI9341_CYAN);
  tft.fillCircle(280, 180, 30, ILI9341_RED);

  Serial.println("Done!");
}

void loop() {
  delay(1000);
}
