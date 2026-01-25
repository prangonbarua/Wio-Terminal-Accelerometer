// Pin Test - Verify which physical pins are being controlled
// Use a multimeter or LED to check which header pins toggle

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=====================================");
  Serial.println("PIN TEST - Check with multimeter/LED");
  Serial.println("=====================================");
  Serial.println();
  Serial.println("Each pin will go HIGH for 3 seconds");
  Serial.println("Check which PHYSICAL header pin toggles");
  Serial.println();

  // Test each pin we're using
  pinMode(0, OUTPUT);  // Should be Pin 13
  pinMode(1, OUTPUT);  // Should be Pin 15
  pinMode(8, OUTPUT);  // Should be Pin 24

  // Also test SPI1 pins explicitly
  pinMode(PIN_SPI1_MOSI, OUTPUT);
  pinMode(PIN_SPI1_SCK, OUTPUT);
}

void loop() {
  Serial.println(">>> Testing D0 (should be Header Pin 13 - RST)");
  Serial.println("    D0 = HIGH for 3 sec...");
  digitalWrite(0, HIGH);
  delay(3000);
  digitalWrite(0, LOW);
  Serial.println("    D0 = LOW");
  delay(1000);

  Serial.println(">>> Testing D1 (should be Header Pin 15 - DC)");
  Serial.println("    D1 = HIGH for 3 sec...");
  digitalWrite(1, HIGH);
  delay(3000);
  digitalWrite(1, LOW);
  Serial.println("    D1 = LOW");
  delay(1000);

  Serial.println(">>> Testing D8 (should be Header Pin 24 - CS)");
  Serial.println("    D8 = HIGH for 3 sec...");
  digitalWrite(8, HIGH);
  delay(3000);
  digitalWrite(8, LOW);
  Serial.println("    D8 = LOW");
  delay(1000);

  Serial.println(">>> Testing PIN_SPI1_MOSI (should be Header Pin 19)");
  Serial.println("    MOSI = HIGH for 3 sec...");
  digitalWrite(PIN_SPI1_MOSI, HIGH);
  delay(3000);
  digitalWrite(PIN_SPI1_MOSI, LOW);
  Serial.println("    MOSI = LOW");
  delay(1000);

  Serial.println(">>> Testing PIN_SPI1_SCK (should be Header Pin 23)");
  Serial.println("    SCK = HIGH for 3 sec...");
  digitalWrite(PIN_SPI1_SCK, HIGH);
  delay(3000);
  digitalWrite(PIN_SPI1_SCK, LOW);
  Serial.println("    SCK = LOW");
  delay(1000);

  Serial.println();
  Serial.println("===== CYCLE COMPLETE - RESTARTING =====");
  Serial.println();
  delay(2000);
}
