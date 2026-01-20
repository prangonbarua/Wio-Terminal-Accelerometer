// SD Card Test for Wio Terminal
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000); // Wait for serial (max 5 seconds)

  delay(1000);
  Serial.println("\n=== SD Card Test ===\n");

  Serial.println("Attempting to initialize SD card...");
  Serial.println("Make sure card is inserted with contacts facing DOWN");
  Serial.println("");

  // Try to initialize
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("ERROR: SD card initialization failed!");
    Serial.println("");
    Serial.println("Possible causes:");
    Serial.println("1. No SD card inserted");
    Serial.println("2. Card not pushed in fully (needs to CLICK)");
    Serial.println("3. Card not formatted as FAT32");
    Serial.println("4. Card is damaged");
    Serial.println("5. Card is too large (try 32GB or smaller)");
    Serial.println("");
    Serial.println("Try:");
    Serial.println("- Remove and reinsert the card");
    Serial.println("- Format as FAT32 on your computer");
    Serial.println("- Try a different SD card");
    return;
  }

  Serial.println("SUCCESS! SD card detected!\n");

  // Get card info
  uint8_t cardType = SD.cardType();
  Serial.print("Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.print("Card Size: ");
  Serial.print((uint32_t)cardSize);
  Serial.println(" MB");

  uint64_t freeSpace = SD.totalBytes() - SD.usedBytes();
  Serial.print("Free Space: ");
  Serial.print((uint32_t)(freeSpace / (1024 * 1024)));
  Serial.println(" MB");

  // Try to create a test file
  Serial.println("\nTrying to create test file...");

  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Hello from Wio Terminal!");
    testFile.close();
    Serial.println("SUCCESS: Created /test.txt");

    // Read it back
    testFile = SD.open("/test.txt");
    if (testFile) {
      Serial.println("File contents:");
      while (testFile.available()) {
        Serial.write(testFile.read());
      }
      testFile.close();
    }
  } else {
    Serial.println("ERROR: Could not create test file");
  }

  Serial.println("\n=== Test Complete ===");
  Serial.println("Your SD card is working!");
}

void loop() {
  delay(10000);
}
