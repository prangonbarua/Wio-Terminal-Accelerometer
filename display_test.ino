// Simple display test for Wio Terminal
#include "TFT_eSPI.h"

TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Starting display test...");

  // Turn on backlight
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);
  Serial.println("Backlight ON");

  // Initialize display
  tft.begin();
  Serial.println("tft.begin() called");

  // Test 1: Fill red
  Serial.println("Filling RED...");
  tft.fillScreen(TFT_RED);
  delay(2000);

  // Test 2: Fill green
  Serial.println("Filling GREEN...");
  tft.fillScreen(TFT_GREEN);
  delay(2000);

  // Test 3: Fill blue
  Serial.println("Filling BLUE...");
  tft.fillScreen(TFT_BLUE);
  delay(2000);

  // Test 4: Draw text
  Serial.println("Drawing text...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(50, 100);
  tft.print("HELLO WIO");

  Serial.println("Test complete!");
}

void loop() {
  // Do nothing
  delay(1000);
  Serial.println("Loop running...");
}
