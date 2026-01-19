#include "TFT_eSPI.h"
#include <TinyGPS++.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

TFT_eSPI tft;
TinyGPSPlus gps;

// Speed tracking (MPH)
float currentSpeed = 0.0;
float peakSpeed = 0.0;
float avgSpeed = 0.0;
float totalSpeed = 0.0;
int speedReadings = 0;

// GPS data
int satellites = 0;
double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;
bool gpsValid = false;

// Trip data
float tripDistance = 0.0; // miles
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;

// Data logging
bool sdCardReady = false;
bool isLogging = false;
File logFile;
String logFileName = "";
unsigned long lastLogTime = 0;
const int LOG_INTERVAL = 1000; // Log every 1 second
int logCount = 0;

// Button pins
const int BUTTON_TOP = WIO_KEY_A;    // Top button - start/stop logging
const int BUTTON_MID = WIO_KEY_B;    // Middle button - reset peak
const int BUTTON_BOT = WIO_KEY_C;    // Bottom button - reset trip

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200; // 5Hz update rate

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); // GPS on Serial1
  delay(1000);

  Serial.println("=== Wio Terminal GPS Logger ===");

  // Setup buttons
  pinMode(BUTTON_TOP, INPUT_PULLUP);
  pinMode(BUTTON_MID, INPUT_PULLUP);
  pinMode(BUTTON_BOT, INPUT_PULLUP);

  // Turn on backlight
  pinMode(72, OUTPUT);
  digitalWrite(72, HIGH);

  // Initialize display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Splash screen
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("GPS SPEEDOMETER");
  tft.setCursor(20, 90);
  tft.print("+ FLIGHT LOGGER");

  // Initialize SD card
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 130);
  tft.print("Initializing SD card...");

  if (SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    sdCardReady = true;
    Serial.println("SD card ready!");
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(20, 145);
    tft.print("SD Card: OK");
  } else {
    sdCardReady = false;
    Serial.println("SD card failed!");
    tft.setTextColor(TFT_RED);
    tft.setCursor(20, 145);
    tft.print("SD Card: NOT FOUND");
  }

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 170);
  tft.print("Waiting for GPS...");

  delay(2000);

  Serial.println("Waiting for GPS signal...");
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // Read GPS data
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Check buttons
  checkButtons();

  // Log data if logging is enabled
  if (isLogging && gpsValid && (millis() - lastLogTime >= LOG_INTERVAL)) {
    logGPSData();
    lastLogTime = millis();
  }

  // Update display at set interval
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    // Update GPS data
    updateGPSData();

    // Draw speedometer
    drawSpeedometerScreen();
  }
}

void checkButtons() {
  // TOP button - start/stop logging
  if (digitalRead(BUTTON_TOP) == LOW) {
    delay(200);
    if (isLogging) {
      stopLogging();
    } else {
      startLogging();
    }
    while (digitalRead(BUTTON_TOP) == LOW);
  }

  // MIDDLE button - reset peak speed
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    peakSpeed = 0.0;
    avgSpeed = 0.0;
    totalSpeed = 0.0;
    speedReadings = 0;
    Serial.println("Peak speed reset");
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // BOTTOM button - reset trip
  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    tripDistance = 0.0;
    tripStarted = false;
    Serial.println("Trip reset");
    while (digitalRead(BUTTON_BOT) == LOW);
  }
}

void startLogging() {
  if (!sdCardReady) {
    Serial.println("Cannot log - no SD card!");
    return;
  }

  // Create unique filename with timestamp
  int fileNum = 1;
  do {
    logFileName = "/flight_" + String(fileNum) + ".csv";
    fileNum++;
  } while (SD.exists(logFileName.c_str()));

  logFile = SD.open(logFileName.c_str(), FILE_WRITE);

  if (logFile) {
    // Write CSV header
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
    Serial.println("File: " + logFileName);
  }
}

void logGPSData() {
  if (!logFile) return;

  // Format: timestamp,lat,lon,altitude,speed,satellites
  String dataLine = "";
  dataLine += String(millis()) + ",";
  dataLine += String(latitude, 6) + ",";
  dataLine += String(longitude, 6) + ",";
  dataLine += String(altitude * 3.28084, 1) + ","; // Convert meters to feet
  dataLine += String(currentSpeed, 1) + ",";
  dataLine += String(satellites);

  logFile.println(dataLine);
  logFile.flush();
  logCount++;

  Serial.println("Logged: " + dataLine);
}

void updateGPSData() {
  if (gps.location.isValid()) {
    gpsValid = true;
    latitude = gps.location.lat();
    longitude = gps.location.lng();

    if (gps.altitude.isValid()) {
      altitude = gps.altitude.meters();
    }

    // Get speed in MPH
    if (gps.speed.isValid()) {
      currentSpeed = gps.speed.mph();

      // Update peak
      if (currentSpeed > peakSpeed) {
        peakSpeed = currentSpeed;
      }

      // Update average
      if (currentSpeed > 0.5) { // Only count when moving
        totalSpeed += currentSpeed;
        speedReadings++;
        avgSpeed = totalSpeed / speedReadings;
      }
    }

    // Calculate trip distance
    if (tripStarted && lastLat != 0.0) {
      double dist = gps.distanceBetween(lastLat, lastLon, latitude, longitude);
      tripDistance += dist / 1609.34; // Convert meters to miles
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

  // GPS status
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
    tft.print(" pts");
  } else if (sdCardReady) {
    tft.setTextColor(TFT_DARKGREY);
    tft.print("SD READY");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO SD");
  }

  // Current speed - BIG
  tft.setTextSize(7);
  if (currentSpeed < 10) {
    tft.setCursor(100, 30);
  } else if (currentSpeed < 100) {
    tft.setCursor(70, 30);
  } else {
    tft.setCursor(40, 30);
  }

  // Color based on speed
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

  // Altitude (for flight tracking)
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(180, 85);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Divider line
  tft.drawLine(10, 105, 310, 105, TFT_DARKGREY);

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 112);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average speed
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 135);
  tft.print("Avg:  ");
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Trip distance
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 158);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // GPS coordinates (small)
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
  if (isLogging) {
    tft.print("[A] STOP LOG");
  } else {
    tft.print("[A] START LOG");
  }
  tft.setCursor(5, 200);
  tft.print("[B] Reset Peak    [C] Reset Trip");

  // File info
  if (isLogging) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(5, 220);
    tft.print("Recording: ");
    tft.print(logFileName);
  }
}
