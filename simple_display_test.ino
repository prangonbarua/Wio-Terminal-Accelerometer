// Simple display test - uses built-in Seeed_Arduino_LCD
#include "TFT_eSPI.h"

TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("=== Simple Display Test ===");

  // Turn on backlight
  pinMode(72, OUTPUT);
  digitalWrite(72, HIGH);
  Serial.println("Backlight ON");

  // Initialize display
  Serial.println("Calling tft.init()...");
  tft.init();
  Serial.println("tft.init() complete");

  Serial.println("Setting rotation...");
  tft.setRotation(3);
  Serial.println("Rotation set");

  delay(1000);

  // Test 1: Fill RED
  Serial.println("\n=== TEST 1: RED ===");
  tft.fillScreen(TFT_RED);
  delay(3000);

  // Test 2: Fill GREEN
  Serial.println("=== TEST 2: GREEN ===");
  tft.fillScreen(TFT_GREEN);
  delay(3000);

  // Test 3: Fill BLUE
  Serial.println("=== TEST 3: BLUE ===");
  tft.fillScreen(TFT_BLUE);
  delay(3000);

  // Test 4: Fill BLACK
  Serial.println("=== TEST 4: BLACK ===");
  tft.fillScreen(TFT_BLACK);
  delay(2000);

  // Test 5: Draw text
  Serial.println("=== TEST 5: TEXT ===");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(50, 100);
  tft.print("HELLO");
  delay(3000);

  Serial.println("\n=== ALL TESTS COMPLETE ===");
  Serial.println("What do you see on the screen?");
}

void loop() {
  delay(10000);
  Serial.println("Loop... What's showing on screen?");
}
