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

// Wio Terminal GPIO mapping:
// Header Pin 13 = D2, Pin 15 = D3, Pin 24 = D8
// Pin 19 = MOSI, Pin 23 = SCK
#define TFT_CS   8   // Header Pin 24
#define TFT_DC   3   // Header Pin 15
#define TFT_RST  2   // Header Pin 13

// Wio Terminal back header uses SPI1 bus (not SPI0 which is for internal LCD)
// Use Hardware SPI1 for the external display
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, &SPI1);
// Parameters: bus, reset pin, rotation (0-3), IPS panel (false for standard)
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RST, 0, false);

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial

  Serial.println("=====================================");
  Serial.println("ILI9488 3.5\" Display Test");
  Serial.println("=====================================");
  Serial.println();

  // Print pin configuration
  Serial.println("Pin Configuration (Hardware SPI1):");
  Serial.print("  CS  = D"); Serial.println(TFT_CS);
  Serial.print("  DC  = D"); Serial.println(TFT_DC);
  Serial.print("  RST = D"); Serial.println(TFT_RST);
  Serial.println("  MOSI = SPI1 MOSI (Header Pin 19)");
  Serial.println("  SCK  = SPI1 SCK (Header Pin 23)");
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

  Serial.println("  Filling RED...");
  gfx->fillScreen(RED);
  delay(500);

  Serial.println("  Filling GREEN...");
  gfx->fillScreen(GREEN);
  delay(500);

  Serial.println("  Filling BLUE...");
  gfx->fillScreen(BLUE);
  delay(500);

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
