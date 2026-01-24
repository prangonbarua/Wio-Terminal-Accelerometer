// External 3.5" ILI9488 Display Test for Wio Terminal
// Using Software SPI to avoid conflicts

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Pin definitions
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9
#define TFT_MOSI 19
#define TFT_CLK  21
#define TFT_MISO 22

// Use SOFTWARE SPI
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Display Test - Software SPI");

  tft.begin();
  Serial.println("tft.begin() done");

  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  Serial.println("fillScreen done");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(30, 50);
  tft.println("HELLO!");

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(30, 100);
  tft.println("Display Working!");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(30, 140);
  tft.println("3.5 inch TFT");

  tft.drawRect(10, 10, 300, 220, ILI9341_CYAN);

  Serial.println("Test complete!");
}

void loop() {
  static bool toggle = false;
  toggle = !toggle;
  tft.fillCircle(280, 200, 15, toggle ? ILI9341_RED : ILI9341_BLACK);
  delay(500);
}
