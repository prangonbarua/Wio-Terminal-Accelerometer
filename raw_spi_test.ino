// Raw SPI Test - Manual bit-bang to verify pins work
// This sends raw commands to try to get ANY response from display

// Try multiple possible pin mappings
#define TFT_CS   PIN_SPI1_SS    // Try the actual SPI1 SS pin
#define TFT_DC   BCM22          // BCM22 should map to header pin 15
#define TFT_RST  BCM27          // BCM27 should map to header pin 13
#define TFT_MOSI PIN_SPI1_MOSI  // Header pin 19
#define TFT_SCK  PIN_SPI1_SCK   // Header pin 23

void sendCommand(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);   // Command mode
  digitalWrite(TFT_CS, LOW);   // Select display

  // Bit-bang the command
  for (int i = 7; i >= 0; i--) {
    digitalWrite(TFT_SCK, LOW);
    digitalWrite(TFT_MOSI, (cmd >> i) & 1);
    digitalWrite(TFT_SCK, HIGH);
  }

  digitalWrite(TFT_CS, HIGH);  // Deselect
}

void sendData(uint8_t data) {
  digitalWrite(TFT_DC, HIGH);  // Data mode
  digitalWrite(TFT_CS, LOW);   // Select display

  // Bit-bang the data
  for (int i = 7; i >= 0; i--) {
    digitalWrite(TFT_SCK, LOW);
    digitalWrite(TFT_MOSI, (data >> i) & 1);
    digitalWrite(TFT_SCK, HIGH);
  }

  digitalWrite(TFT_CS, HIGH);  // Deselect
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=====================================");
  Serial.println("RAW SPI TEST - Manual Bit-Bang");
  Serial.println("=====================================");

  // Print actual pin values
  Serial.print("TFT_CS (PIN_SPI1_SS) = "); Serial.println(TFT_CS);
  Serial.print("TFT_DC (BCM22) = "); Serial.println(TFT_DC);
  Serial.print("TFT_RST (BCM27) = "); Serial.println(TFT_RST);
  Serial.print("TFT_MOSI (PIN_SPI1_MOSI) = "); Serial.println(TFT_MOSI);
  Serial.print("TFT_SCK (PIN_SPI1_SCK) = "); Serial.println(TFT_SCK);
  Serial.println();

  // Configure pins
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_MOSI, OUTPUT);
  pinMode(TFT_SCK, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_SCK, LOW);

  // Hardware reset
  Serial.println("Performing hardware reset...");
  digitalWrite(TFT_RST, HIGH);
  delay(50);
  digitalWrite(TFT_RST, LOW);
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(150);
  Serial.println("Reset done.");

  // ILI9488 initialization sequence
  Serial.println("Sending ILI9488 init commands...");

  // Software reset
  sendCommand(0x01);
  delay(150);

  // Sleep out
  sendCommand(0x11);
  delay(150);

  // Pixel format - 18bit color
  sendCommand(0x3A);
  sendData(0x66);  // 18-bit color

  // Memory access control
  sendCommand(0x36);
  sendData(0x48);

  // Display ON
  sendCommand(0x29);
  delay(50);

  Serial.println("Init complete!");
  Serial.println();

  // Fill screen with RED
  Serial.println("Attempting to fill RED...");

  // Set column address (0 to 479)
  sendCommand(0x2A);
  sendData(0x00); sendData(0x00);  // Start col
  sendData(0x01); sendData(0xDF);  // End col (479)

  // Set row address (0 to 319)
  sendCommand(0x2B);
  sendData(0x00); sendData(0x00);  // Start row
  sendData(0x01); sendData(0x3F);  // End row (319)

  // Write memory
  sendCommand(0x2C);

  // Send red pixels (18-bit: R=0xFF, G=0x00, B=0x00)
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);

  for (long i = 0; i < 480L * 320L; i++) {
    // Send RGB (6 bits each, shifted to 8 bits)
    // Red
    for (int b = 7; b >= 0; b--) {
      digitalWrite(TFT_SCK, LOW);
      digitalWrite(TFT_MOSI, (0xFC >> b) & 1);  // Red = 0xFC
      digitalWrite(TFT_SCK, HIGH);
    }
    // Green
    for (int b = 7; b >= 0; b--) {
      digitalWrite(TFT_SCK, LOW);
      digitalWrite(TFT_MOSI, 0);  // Green = 0
      digitalWrite(TFT_SCK, HIGH);
    }
    // Blue
    for (int b = 7; b >= 0; b--) {
      digitalWrite(TFT_SCK, LOW);
      digitalWrite(TFT_MOSI, 0);  // Blue = 0
      digitalWrite(TFT_SCK, HIGH);
    }

    if (i % 10000 == 0) {
      Serial.print(".");
    }
  }

  digitalWrite(TFT_CS, HIGH);

  Serial.println();
  Serial.println("Done! Check display for RED screen.");
  Serial.println("=====================================");
}

void loop() {
  delay(1000);
}
