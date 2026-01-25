// Pin Check - Print actual pin numbers on Wio Terminal

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("=====================================");
  Serial.println("WIO TERMINAL PIN CHECK");
  Serial.println("=====================================");
  Serial.println();

  // Check if BCM defines exist
  #ifdef BCM27
    Serial.print("BCM27 = "); Serial.println(BCM27);
  #else
    Serial.println("BCM27 NOT DEFINED");
  #endif

  #ifdef BCM22
    Serial.print("BCM22 = "); Serial.println(BCM22);
  #else
    Serial.println("BCM22 NOT DEFINED");
  #endif

  #ifdef BCM23
    Serial.print("BCM23 = "); Serial.println(BCM23);
  #else
    Serial.println("BCM23 NOT DEFINED");
  #endif

  #ifdef BCM24
    Serial.print("BCM24 = "); Serial.println(BCM24);
  #else
    Serial.println("BCM24 NOT DEFINED");
  #endif

  Serial.println();

  // Check SPI1 pins
  #ifdef PIN_SPI1_MOSI
    Serial.print("PIN_SPI1_MOSI = "); Serial.println(PIN_SPI1_MOSI);
  #else
    Serial.println("PIN_SPI1_MOSI NOT DEFINED");
  #endif

  #ifdef PIN_SPI1_MISO
    Serial.print("PIN_SPI1_MISO = "); Serial.println(PIN_SPI1_MISO);
  #else
    Serial.println("PIN_SPI1_MISO NOT DEFINED");
  #endif

  #ifdef PIN_SPI1_SCK
    Serial.print("PIN_SPI1_SCK = "); Serial.println(PIN_SPI1_SCK);
  #else
    Serial.println("PIN_SPI1_SCK NOT DEFINED");
  #endif

  #ifdef PIN_SPI1_SS
    Serial.print("PIN_SPI1_SS = "); Serial.println(PIN_SPI1_SS);
  #else
    Serial.println("PIN_SPI1_SS NOT DEFINED");
  #endif

  Serial.println();

  // Standard D pins
  Serial.println("Standard D pins:");
  Serial.print("D0 = "); Serial.println(D0);
  Serial.print("D1 = "); Serial.println(D1);
  Serial.print("D2 = "); Serial.println(D2);
  Serial.print("D3 = "); Serial.println(D3);
  Serial.print("D4 = "); Serial.println(D4);
  Serial.print("D5 = "); Serial.println(D5);
  Serial.print("D6 = "); Serial.println(D6);
  Serial.print("D7 = "); Serial.println(D7);
  Serial.print("D8 = "); Serial.println(D8);

  Serial.println();
  Serial.println("=====================================");
  Serial.println("Copy these values and send to me!");
  Serial.println("=====================================");
}

void loop() {
  delay(5000);
  Serial.println("Waiting for you to copy the values above...");
}
