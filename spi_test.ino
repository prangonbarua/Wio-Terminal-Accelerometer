// SPI3 test for Wio Terminal LCD
#include "TFT_eSPI.h"
#include <SPI.h>

TFT_eSPI tft;

// Define SPI3 (used by LCD on Wio Terminal)
SPIClass SPI3(&sercom7, 66, 68, 67, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_2);

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("=== SPI3 Initialization Test ===");

  // Configure SPI3 pins
  Serial.println("Configuring SPI3 pins...");
  pinPeripheral(66, PIO_SERCOM);  // MISO
  pinPeripheral(67, PIO_SERCOM);  // MOSI
  pinPeripheral(68, PIO_SERCOM);  // SCK
  Serial.println("SPI3 pins configured");

  // Initialize SPI3
  Serial.println("Initializing SPI3...");
  SPI3.begin();
  Serial.println("SPI3 initialized");

  // Reset display
  Serial.println("Resetting display...");
  pinMode(71, OUTPUT);  // LCD_RESET
  digitalWrite(71, LOW);
  delay(100);
  digitalWrite(71, HIGH);
  delay(200);
  Serial.println("Display reset done");

  // Turn on backlight
  pinMode(72, OUTPUT);  // LCD_BACKLIGHT
  digitalWrite(72, HIGH);
  Serial.println("Backlight ON");

  delay(500);

  // Now initialize TFT
  Serial.println("Initializing TFT...");
  tft.begin();
  tft.setRotation(3);
  Serial.println("TFT initialized");

  // Test colors
  Serial.println("\nTEST 1: RED");
  tft.fillScreen(TFT_RED);
  delay(2000);

  Serial.println("TEST 2: GREEN");
  tft.fillScreen(TFT_GREEN);
  delay(2000);

  Serial.println("TEST 3: BLUE");
  tft.fillScreen(TFT_BLUE);
  delay(2000);

  Serial.println("TEST 4: BLACK + WHITE TEXT");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(20, 100);
  tft.print("WIO TEST");

  Serial.println("\nTests complete!");
  Serial.println("What do you see on screen?");
}

void loop() {
  delay(5000);
  Serial.println("Running...");
}

// Required for SPI3
void SERCOM7_0_Handler() {
  SPI3.onService();
}
void SERCOM7_1_Handler() {
  SPI3.onService();
}
void SERCOM7_2_Handler() {
  SPI3.onService();
}
void SERCOM7_3_Handler() {
  SPI3.onService();
}
