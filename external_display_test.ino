// External 3.5" ILI9488 Display Test for Wio Terminal
// Tests the external SPI display

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>  // Works for ILI9488 too

// Pin definitions for external display
#define TFT_CS     10  // Pin 13 on Wio header (D10)
#define TFT_RST    9   // Pin 15 on Wio header (D9)
#define TFT_DC     8   // Pin 17 on Wio header (D8)

// Create display object
Adafruit_ILI9341 tft_ext = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("External Display Test");

  // Initialize external display
  tft_ext.begin();
  tft_ext.setRotation(1);  // Landscape
  tft_ext.fillScreen(ILI9341_BLACK);

  // Draw test pattern
  tft_ext.setTextColor(ILI9341_WHITE);
  tft_ext.setTextSize(3);
  tft_ext.setCursor(50, 50);
  tft_ext.println("DISPLAY OK!");

  tft_ext.setTextSize(2);
  tft_ext.setTextColor(ILI9341_GREEN);
  tft_ext.setCursor(50, 120);
  tft_ext.println("External 3.5\" TFT");

  tft_ext.setTextColor(ILI9341_YELLOW);
  tft_ext.setCursor(50, 160);
  tft_ext.println("ILI9488 SPI");

  // Draw some shapes
  tft_ext.drawRect(10, 10, 300, 220, ILI9341_CYAN);
  tft_ext.fillCircle(400, 120, 50, ILI9341_RED);

  Serial.println("Display initialized!");
}

void loop() {
  // Blink a rectangle to show it's running
  static bool toggle = false;
  toggle = !toggle;

  if (toggle) {
    tft_ext.fillRect(420, 200, 50, 30, ILI9341_GREEN);
  } else {
    tft_ext.fillRect(420, 200, 50, 30, ILI9341_BLACK);
  }

  delay(500);
}
