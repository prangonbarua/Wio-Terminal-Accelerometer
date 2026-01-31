/*
 * Wio Terminal GPS Speedometer + Flight Logger
 * WITH GPS DEBUG MODE
 *
 * Wiring:
 *   GPS TX  -> Wio Pin 8 (RX)
 *   GPS RX  -> Wio Pin 10 (TX) [optional]
 *   GPS VCC -> 3.3V
 *   GPS GND -> GND
 *
 * If using Grove GPS: Just plug into the right Grove port (UART)
 */

#include "TFT_eSPI.h"
#include <TinyGPS++.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

TFT_eSPI tft;
TinyGPSPlus gps;

// ============ GPS DEBUG ============
bool debugMode = true;  // Set to false after GPS works
int bytesReceived = 0;
int validSentences = 0;
unsigned long lastByteTime = 0;
char lastChars[50] = "";
int charIndex = 0;
int currentBaudRate = 9600;

// Baud rates to try
const long baudRates[] = {9600, 38400, 57600, 115200, 4800};
const int numBaudRates = 5;
int baudIndex = 0;

// ============ SPEED TRACKING ============
float currentSpeed = 0.0;
float peakSpeed = 0.0;
float avgSpeed = 0.0;
float totalSpeed = 0.0;
int speedReadings = 0;

// ============ GPS DATA ============
int satellites = 0;
double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;
bool gpsValid = false;
bool gpsDataReceived = false;

// ============ TRIP DATA ============
float tripDistance = 0.0;
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;

// ============ DATA LOGGING ============
bool sdCardReady = false;
bool isLogging = false;
File logFile;
String logFileName = "";
unsigned long lastLogTime = 0;
const int LOG_INTERVAL = 1000;
int logCount = 0;

// ============ BUTTONS ============
const int BUTTON_TOP = WIO_KEY_A;
const int BUTTON_MID = WIO_KEY_B;
const int BUTTON_BOT = WIO_KEY_C;

// ============ TIMING ============
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200;
unsigned long gpsCheckStart = 0;
const int BAUD_SWITCH_TIME = 5000; // Try each baud rate for 5 seconds

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("===========================================");
  Serial.println("   Wio Terminal GPS Speedometer Debug");
  Serial.println("===========================================");

  // Setup buttons
  pinMode(BUTTON_TOP, INPUT_PULLUP);
  pinMode(BUTTON_MID, INPUT_PULLUP);
  pinMode(BUTTON_BOT, INPUT_PULLUP);

  // Backlight ON
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);

  // Initialize display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Splash
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(30, 40);
  tft.print("GPS SPEEDOMETER");
  tft.setCursor(50, 65);
  tft.setTextColor(TFT_YELLOW);
  tft.print("DEBUG MODE");

  // Initialize SD card
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 100);
  tft.print("SD Card: ");

  if (SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    sdCardReady = true;
    tft.setTextColor(TFT_GREEN);
    tft.print("OK");
    Serial.println("SD card: OK");
  } else {
    sdCardReady = false;
    tft.setTextColor(TFT_RED);
    tft.print("NOT FOUND");
    Serial.println("SD card: NOT FOUND");
  }

  // Start GPS serial
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 115);
  tft.print("Starting GPS at 9600 baud...");

  Serial1.begin(9600);
  currentBaudRate = 9600;
  gpsCheckStart = millis();

  Serial.println("GPS Serial started at 9600 baud");
  Serial.println("Waiting for GPS data...");
  Serial.println("");

  delay(1500);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // Read all available GPS data
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    bytesReceived++;
    lastByteTime = millis();
    gpsDataReceived = true;

    // Store last characters for debug display
    if (charIndex < 49) {
      lastChars[charIndex++] = c;
      lastChars[charIndex] = '\0';
    }

    // Feed to TinyGPS++
    if (gps.encode(c)) {
      validSentences++;
    }

    // Also print raw to Serial for debugging
    Serial.print(c);
  }

  // Auto baud rate detection (if no data received)
  if (debugMode && !gpsDataReceived && (millis() - gpsCheckStart > BAUD_SWITCH_TIME)) {
    tryNextBaudRate();
  }

  // Check buttons
  checkButtons();

  // Log data if enabled
  if (isLogging && gpsValid && (millis() - lastLogTime >= LOG_INTERVAL)) {
    logGPSData();
    lastLogTime = millis();
  }

  // Update display
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateGPSData();

    if (debugMode) {
      drawDebugScreen();
    } else {
      drawSpeedometerScreen();
    }
  }
}

void tryNextBaudRate() {
  baudIndex = (baudIndex + 1) % numBaudRates;
  currentBaudRate = baudRates[baudIndex];

  Serial.println("");
  Serial.print("Trying baud rate: ");
  Serial.println(currentBaudRate);

  Serial1.end();
  delay(100);
  Serial1.begin(currentBaudRate);

  bytesReceived = 0;
  validSentences = 0;
  charIndex = 0;
  lastChars[0] = '\0';
  gpsDataReceived = false;
  gpsCheckStart = millis();
}

void checkButtons() {
  // TOP button - Toggle debug mode / start-stop logging
  if (digitalRead(BUTTON_TOP) == LOW) {
    delay(200);
    if (debugMode && gpsDataReceived) {
      // Exit debug mode if GPS is working
      debugMode = false;
      tft.fillScreen(TFT_BLACK);
      Serial.println("Debug mode OFF - GPS working!");
    } else if (!debugMode) {
      // Toggle logging
      if (isLogging) {
        stopLogging();
      } else {
        startLogging();
      }
    }
    while (digitalRead(BUTTON_TOP) == LOW);
  }

  // MIDDLE button - Reset peak / Force try next baud rate
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    if (debugMode) {
      tryNextBaudRate();
    } else {
      peakSpeed = 0.0;
      avgSpeed = 0.0;
      totalSpeed = 0.0;
      speedReadings = 0;
      Serial.println("Peak speed reset");
    }
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // BOTTOM button - Reset trip / Back to debug
  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    if (debugMode) {
      // Clear buffer and restart
      charIndex = 0;
      lastChars[0] = '\0';
      bytesReceived = 0;
      validSentences = 0;
    } else {
      tripDistance = 0.0;
      tripStarted = false;
      Serial.println("Trip reset");
    }
    while (digitalRead(BUTTON_BOT) == LOW);
  }
}

void drawDebugScreen() {
  tft.fillScreen(TFT_BLACK);

  // Title
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(50, 5);
  tft.print("GPS DEBUG MODE");

  // Baud rate
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 30);
  tft.print("Baud Rate: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(currentBaudRate);

  // Bytes received
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 45);
  tft.print("Bytes Received: ");
  if (bytesReceived > 0) {
    tft.setTextColor(TFT_GREEN);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(bytesReceived);

  // Valid sentences
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 60);
  tft.print("Valid NMEA: ");
  if (validSentences > 0) {
    tft.setTextColor(TFT_GREEN);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(validSentences);

  // Satellites
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 75);
  tft.print("Satellites: ");
  if (satellites > 0) {
    tft.setTextColor(TFT_GREEN);
  } else {
    tft.setTextColor(TFT_ORANGE);
  }
  tft.print(satellites);

  // GPS Fix status
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 90);
  tft.print("GPS Fix: ");
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("YES - LOCKED!");
  } else if (gpsDataReceived) {
    tft.setTextColor(TFT_YELLOW);
    tft.print("Waiting for fix...");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO DATA");
  }

  // Time since last byte
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 105);
  tft.print("Last data: ");
  if (lastByteTime > 0) {
    unsigned long ago = millis() - lastByteTime;
    if (ago < 1000) {
      tft.setTextColor(TFT_GREEN);
      tft.print(ago);
      tft.print("ms ago");
    } else {
      tft.setTextColor(TFT_RED);
      tft.print(ago / 1000);
      tft.print("s ago");
    }
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("Never");
  }

  // Raw data preview
  tft.drawLine(5, 120, 315, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 125);
  tft.print("Raw GPS Data:");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 138);
  if (charIndex > 0) {
    // Show last received characters (truncated to fit)
    String preview = String(lastChars);
    preview.replace('\n', ' ');
    preview.replace('\r', ' ');
    if (preview.length() > 45) {
      preview = preview.substring(preview.length() - 45);
    }
    tft.print(preview);
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("No data received");
  }

  // Coordinates if available
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 155);
    tft.print("LAT: ");
    tft.print(latitude, 6);
    tft.setCursor(10, 167);
    tft.print("LON: ");
    tft.print(longitude, 6);
  }

  // Instructions
  tft.drawLine(5, 180, 315, 180, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 188);
  if (gpsDataReceived && validSentences > 0) {
    tft.setTextColor(TFT_GREEN);
    tft.print("[A] GPS Working! Press to continue");
  } else {
    tft.print("[A] Exit debug  [B] Try next baud");
  }
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(10, 200);
  tft.print("[C] Clear buffer");

  // Status indicator
  tft.setCursor(10, 218);
  if (!gpsDataReceived) {
    tft.setTextColor(TFT_RED);
    tft.print("!! CHECK WIRING: GPS TX -> Wio Pin 8 !!");
  } else if (validSentences == 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.print("Data received but not valid NMEA");
  } else if (!gpsValid) {
    tft.setTextColor(TFT_YELLOW);
    tft.print("NMEA valid - waiting for satellite fix");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS FULLY WORKING!");
  }
}

void startLogging() {
  if (!sdCardReady) {
    Serial.println("Cannot log - no SD card!");
    return;
  }

  int fileNum = 1;
  do {
    logFileName = "/flight_" + String(fileNum) + ".csv";
    fileNum++;
  } while (SD.exists(logFileName.c_str()));

  logFile = SD.open(logFileName.c_str(), FILE_WRITE);

  if (logFile) {
    logFile.println("timestamp,latitude,longitude,altitude_ft,speed_mph,satellites");
    logFile.flush();
    isLogging = true;
    logCount = 0;
    Serial.println("Logging started: " + logFileName);
  } else {
    Serial.println("Failed to create log file!");
  }
}

void stopLogging() {
  if (isLogging && logFile) {
    logFile.close();
    isLogging = false;
    Serial.println("Logging stopped. " + String(logCount) + " points saved.");
  }
}

void logGPSData() {
  if (!logFile) return;

  String dataLine = "";
  dataLine += String(millis()) + ",";
  dataLine += String(latitude, 6) + ",";
  dataLine += String(longitude, 6) + ",";
  dataLine += String(altitude * 3.28084, 1) + ",";
  dataLine += String(currentSpeed, 1) + ",";
  dataLine += String(satellites);

  logFile.println(dataLine);
  logFile.flush();
  logCount++;
}

void updateGPSData() {
  if (gps.location.isValid()) {
    gpsValid = true;
    latitude = gps.location.lat();
    longitude = gps.location.lng();

    if (gps.altitude.isValid()) {
      altitude = gps.altitude.meters();
    }

    if (gps.speed.isValid()) {
      currentSpeed = gps.speed.mph();

      if (currentSpeed > peakSpeed) {
        peakSpeed = currentSpeed;
      }

      if (currentSpeed > 0.5) {
        totalSpeed += currentSpeed;
        speedReadings++;
        avgSpeed = totalSpeed / speedReadings;
      }
    }

    if (tripStarted && lastLat != 0.0) {
      double dist = gps.distanceBetween(lastLat, lastLon, latitude, longitude);
      tripDistance += dist / 1609.34;
    }

    lastLat = latitude;
    lastLon = longitude;
    tripStarted = true;

  } else {
    gpsValid = false;
  }

  satellites = gps.satellites.value();
}

void drawSpeedometerScreen() {
  tft.fillScreen(TFT_BLACK);

  // GPS status bar
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS LOCKED ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("SEARCHING ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

  // Logging status
  tft.setCursor(200, 5);
  if (isLogging) {
    tft.setTextColor(TFT_RED);
    tft.print("REC ");
    tft.setTextColor(TFT_WHITE);
    tft.print(logCount);
  } else if (sdCardReady) {
    tft.setTextColor(TFT_DARKGREY);
    tft.print("SD READY");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO SD");
  }

  // BIG Speed display
  tft.setTextSize(7);
  int xPos = (currentSpeed < 10) ? 100 : (currentSpeed < 100) ? 70 : 40;
  tft.setCursor(xPos, 30);

  if (currentSpeed < 30) {
    tft.setTextColor(TFT_GREEN);
  } else if (currentSpeed < 60) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(currentSpeed, 0);

  // MPH label
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(230, 55);
  tft.print("MPH");

  // Altitude
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(180, 85);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Divider
  tft.drawLine(10, 105, 310, 105, TFT_DARKGREY);

  // Stats
  tft.setTextSize(2);

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 112);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 135);
  tft.print("Avg:  ");
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 158);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // Coordinates
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(170, 115);
    tft.print("LAT: ");
    tft.print(latitude, 5);
    tft.setCursor(170, 127);
    tft.print("LON: ");
    tft.print(longitude, 5);
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 185);
  tft.print(isLogging ? "[A] STOP LOG" : "[A] START LOG");
  tft.setCursor(5, 200);
  tft.print("[B] Reset Peak    [C] Reset Trip");

  if (isLogging) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(5, 220);
    tft.print("Recording: ");
    tft.print(logFileName);
  }
}
