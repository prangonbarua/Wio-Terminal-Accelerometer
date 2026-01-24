// External 3.5" ILI9488 Display Test
// Using Arduino_GFX with correct ILI9488 driver

#include <Arduino_GFX_Library.h>

// Wio Terminal GPIO mapping:
// Header Pin 13 = D2, Pin 15 = D3, Pin 24 = D8
// Pin 19 = MOSI, Pin 23 = SCK
#define TFT_CS   8   // Header Pin 24
#define TFT_DC   3   // Header Pin 15
#define TFT_RST  2   // Header Pin 13

// Hardware SPI bus + ILI9488 driver
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RST, 1, false);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ILI9488 Display Test");

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    while(1) delay(100);
  }
  Serial.println("gfx->begin() OK!");

  gfx->fillScreen(BLACK);
  Serial.println("fillScreen done");

  gfx->setTextColor(WHITE);
  gfx->setTextSize(4);
  gfx->setCursor(50, 50);
  gfx->println("HELLO!");

  gfx->setTextColor(GREEN);
  gfx->setTextSize(3);
  gfx->setCursor(50, 120);
  gfx->println("ILI9488 Working!");

  gfx->drawRect(10, 10, 460, 300, CYAN);
  gfx->fillCircle(400, 200, 50, RED);

  Serial.println("Done!");
}

void loop() {
  delay(1000);
}
