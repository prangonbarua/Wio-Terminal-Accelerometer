// ILI9488 Test using dedicated ILI9488 library by Jaret Burkett
// Install: Arduino IDE -> Library Manager -> Search "ILI9488" -> Install by Jaret Burkett
// Also needs: Adafruit GFX Library

#include <SPI.h>
#include "ILI9488.h"

// Wio Terminal back header pins
#define TFT_CS   8   // Header Pin 24
#define TFT_DC   1   // Header Pin 15
#define TFT_RST  0   // Header Pin 13
// MOSI = Pin 19 (hardware SPI1)
// SCK = Pin 23 (hardware SPI1)

// Create display object
ILI9488 tft = ILI9488(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=====================================");
  Serial.println("ILI9488 Test - Jaret Burkett Library");
  Serial.println("=====================================");
  Serial.println();

  Serial.println("Wiring:");
  Serial.println("  VCC  -> Pin 1 (3.3V)");
  Serial.println("  GND  -> Pin 6 (GND)");
  Serial.println("  CS   -> Pin 24");
  Serial.println("  RST  -> Pin 13");
  Serial.println("  DC   -> Pin 15");
  Serial.println("  MOSI -> Pin 19");
  Serial.println("  SCK  -> Pin 23");
  Serial.println("  LED  -> Pin 2 or 4 (5V)");
  Serial.println();

  Serial.println("Initializing display...");
  tft.begin();
  Serial.println("Display initialized!");

  // Test colors
  Serial.println("Filling RED...");
  tft.fillScreen(ILI9488_RED);
  delay(2000);

  Serial.println("Filling GREEN...");
  tft.fillScreen(ILI9488_GREEN);
  delay(2000);

  Serial.println("Filling BLUE...");
  tft.fillScreen(ILI9488_BLUE);
  delay(2000);

  Serial.println("Filling BLACK...");
  tft.fillScreen(ILI9488_BLACK);
  delay(500);

  // Draw text
  tft.setTextColor(ILI9488_WHITE);
  tft.setTextSize(4);
  tft.setCursor(50, 50);
  tft.println("HELLO!");

  tft.setTextColor(ILI9488_GREEN);
  tft.setTextSize(3);
  tft.setCursor(50, 120);
  tft.println("ILI9488 Working!");

  tft.setTextColor(ILI9488_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(50, 180);
  tft.print("480x320 Display");

  // Draw shapes
  tft.fillCircle(400, 250, 40, ILI9488_RED);
  tft.fillRect(20, 250, 80, 50, ILI9488_BLUE);

  Serial.println();
  Serial.println("Test complete! Check display.");
  Serial.println("=====================================");
}

void loop() {
  // Blink indicator
  static bool toggle = false;
  toggle = !toggle;
  tft.fillRect(5, 5, 10, 10, toggle ? ILI9488_WHITE : ILI9488_BLACK);
  delay(500);
}
