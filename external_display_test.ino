// External 3.5" ILI9488 Display Test
// Using Arduino_GFX with correct ILI9488 driver
// LCD Wiki: https://www.lcdwiki.com/3.5inch_SPI_Module_ILI9488_SKU:MSP3520

#include <Arduino_GFX_Library.h>
#include <SPI.h>

// Colors (RGB565 format)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F

// Wio Terminal GPIO mapping (CORRECTED):
// Header Pin 13 (BCM27) = D0
// Header Pin 15 (BCM22) = D1
// Header Pin 24 (CE0) = D8 / SPI1 SS
// Pin 19 = SPI1 MOSI, Pin 23 = SPI1 SCK
#define TFT_CS   8   // Header Pin 24
#define TFT_DC   1   // Header Pin 15 (BCM22 = D1)
#define TFT_RST  0   // Header Pin 13 (BCM27 = D0)

// Try SOFTWARE SPI for debugging - explicit pin control
// We need to find which Arduino pins map to Header Pin 19 (MOSI) and Pin 23 (SCK)
// According to Wio Terminal docs: these are SPI1 pins
// Let's use the SPI1 pin definitions directly
#define TFT_MOSI PIN_SPI1_MOSI  // Header Pin 19
#define TFT_SCK  PIN_SPI1_SCK   // Header Pin 23

// Software SPI - manually bit-bang the data
Arduino_DataBus *bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, -1);

// ILI9488 18-bit driver - try with IPS=true
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RST, 0, true);

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial

  Serial.println("=====================================");
  Serial.println("ILI9488 3.5\" Display Test");
  Serial.println("=====================================");
  Serial.println();

  // Print pin configuration
  Serial.println("Pin Configuration (Software SPI):");
  Serial.print("  CS   = "); Serial.println(TFT_CS);
  Serial.print("  DC   = "); Serial.println(TFT_DC);
  Serial.print("  RST  = "); Serial.println(TFT_RST);
  Serial.print("  MOSI = "); Serial.print(TFT_MOSI); Serial.println(" (PIN_SPI1_MOSI)");
  Serial.print("  SCK  = "); Serial.print(TFT_SCK); Serial.println(" (PIN_SPI1_SCK)");
  Serial.println();

  // Manual reset sequence
  Serial.println("Performing manual reset...");
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(200);
  Serial.println("Reset complete.");

  // Initialize display
  Serial.println("Calling gfx->begin()...");
  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed!");
    Serial.println("Check wiring and try again.");
    while(1) {
      delay(500);
    }
  }
  Serial.println("SUCCESS: gfx->begin() OK!");
  Serial.println();

  // Clear screen with different colors to test
  Serial.println("Testing colors...");

  Serial.println("  Filling RED... (2 sec)");
  gfx->fillScreen(RED);
  delay(2000);

  Serial.println("  Filling GREEN... (2 sec)");
  gfx->fillScreen(GREEN);
  delay(2000);

  Serial.println("  Filling BLUE... (2 sec)");
  gfx->fillScreen(BLUE);
  delay(2000);

  Serial.println("  Filling WHITE... (2 sec)");
  gfx->fillScreen(WHITE);
  delay(2000);

  Serial.println("  Filling BLACK...");
  gfx->fillScreen(BLACK);
  delay(200);

  // Draw test pattern
  Serial.println("Drawing test pattern...");

  // Border rectangle
  gfx->drawRect(0, 0, gfx->width(), gfx->height(), WHITE);
  gfx->drawRect(5, 5, gfx->width()-10, gfx->height()-10, CYAN);

  // Text
  gfx->setTextColor(WHITE);
  gfx->setTextSize(4);
  gfx->setCursor(50, 50);
  gfx->println("HELLO!");

  gfx->setTextColor(GREEN);
  gfx->setTextSize(3);
  gfx->setCursor(50, 120);
  gfx->println("ILI9488 Working!");

  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->setCursor(50, 180);
  gfx->print("Resolution: ");
  gfx->print(gfx->width());
  gfx->print("x");
  gfx->println(gfx->height());

  // Shapes
  gfx->fillCircle(400, 250, 40, RED);
  gfx->fillRect(20, 250, 80, 50, BLUE);
  gfx->fillTriangle(150, 300, 200, 220, 250, 300, MAGENTA);

  Serial.println();
  Serial.println("=====================================");
  Serial.println("Test complete! Check display.");
  Serial.println("=====================================");
}

void loop() {
  // Blink a pixel to show loop is running
  static bool toggle = false;
  toggle = !toggle;
  gfx->drawPixel(gfx->width()-5, 5, toggle ? WHITE : BLACK);
  delay(500);
}
