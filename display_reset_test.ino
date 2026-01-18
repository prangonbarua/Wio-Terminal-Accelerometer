// Display reset and test for Wio Terminal
#include "TFT_eSPI.h"

TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=== Display Reset Test ===");

  // Reset display first
  Serial.println("Resetting display...");
  pinMode(LCD_RESET, OUTPUT);
  digitalWrite(LCD_RESET, LOW);
  delay(100);
  digitalWrite(LCD_RESET, HIGH);
  delay(100);
  Serial.println("Display reset complete");

  // Turn on backlight
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);
  Serial.println("Backlight ON");

  delay(500);

  // Initialize display
  Serial.println("Initializing display...");
  tft.begin();
  tft.setRotation(3);
  Serial.println("Display initialized");

  delay(500);

  // Try different color fills with delays
  Serial.println("TEST 1: Filling BLACK...");
  tft.fillScreen(0x0000);  // Black
  delay(2000);

  Serial.println("TEST 2: Filling RED...");
  tft.fillScreen(0xF800);  // Red
  delay(2000);

  Serial.println("TEST 3: Filling GREEN...");
  tft.fillScreen(0x07E0);  // Green
  delay(2000);

  Serial.println("TEST 4: Filling BLUE...");
  tft.fillScreen(0x001F);  // Blue
  delay(2000);

  Serial.println("TEST 5: Filling YELLOW...");
  tft.fillScreen(0xFFE0);  // Yellow
  delay(2000);

  Serial.println("TEST 6: Filling WHITE...");
  tft.fillScreen(0xFFFF);  // White
  delay(2000);

  Serial.println("TEST 7: Drawing rectangle...");
  tft.fillScreen(0x0000);  // Black background
  tft.fillRect(50, 50, 100, 100, 0xF800);  // Red rectangle
  delay(2000);

  Serial.println("TEST 8: Drawing text...");
  tft.fillScreen(0x0000);  // Black
  tft.setTextColor(0xFFFF);  // White text
  tft.setTextSize(4);
  tft.setCursor(50, 100);
  tft.print("HELLO");

  Serial.println("All tests complete!");
  Serial.println("What do you see on the screen?");
}

void loop() {
  delay(5000);
  Serial.println("Still running... What's on screen?");
}
