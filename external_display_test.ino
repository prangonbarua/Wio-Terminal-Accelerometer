// External 3.5" ILI9488 Display Test for Wio Terminal
// Using Arduino_GFX library

#include <Arduino_GFX_Library.h>

// Define colors (RGB565 format)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F

// Pin definitions for external display
#define TFT_CS   10  // Pin 13 on Wio header
#define TFT_DC   8   // Pin 17 on Wio header
#define TFT_RST  9   // Pin 15 on Wio header

// Create display - ILI9488 480x320
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RST, 1 /* rotation */);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("External Display Test - ILI9488");

  // Initialize display
  if (!gfx->begin()) {
    Serial.println("Display init FAILED!");
    while (1);
  }
  Serial.println("Display init OK!");

  // Clear screen
  gfx->fillScreen(BLACK);

  // Draw test
  gfx->setTextColor(WHITE);
  gfx->setTextSize(4);
  gfx->setCursor(50, 50);
  gfx->println("DISPLAY OK!");

  gfx->setTextSize(3);
  gfx->setTextColor(GREEN);
  gfx->setCursor(50, 120);
  gfx->println("3.5 inch ILI9488");

  gfx->setTextColor(YELLOW);
  gfx->setCursor(50, 170);
  gfx->println("480 x 320 SPI");

  // Draw shapes
  gfx->drawRect(10, 10, 460, 300, CYAN);
  gfx->fillCircle(400, 250, 40, RED);

  Serial.println("Test complete!");
}

void loop() {
  // Blink indicator
  static bool toggle = false;
  toggle = !toggle;

  gfx->fillRect(420, 10, 50, 30, toggle ? GREEN : BLACK);
  delay(500);
}
