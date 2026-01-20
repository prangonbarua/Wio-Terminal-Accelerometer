// Wio Terminal GPS WiFi Tracker
// Sends GPS data to server for live map tracking
// Press TOP button to switch between Speedometer/Stats modes
// GPS data is continuously sent to server over WiFi

#include "TFT_eSPI.h"
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>

TFT_eSPI tft;
TinyGPSPlus gps;

// ============================================
// CONFIGURE THESE SETTINGS
// ============================================
const char* ssid = "WifiPrajoy";
const char* password = "S09273788";
const char* serverURL = "http://10.0.0.93:5002/api/gps/update";
const char* deviceId = "wio_default";
// ============================================

// Modes
enum Mode { SPEEDOMETER, STATS };
Mode currentMode = SPEEDOMETER;

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
float tripDistance = 0.0;
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;
unsigned long tripStartTime = 0;

// WiFi status
bool wifiConnected = false;
int uploadCount = 0;
int uploadErrors = 0;

// Button pins
const int BUTTON_TOP = WIO_KEY_A;
const int BUTTON_MID = WIO_KEY_B;
const int BUTTON_BOT = WIO_KEY_C;

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200;
unsigned long lastUpload = 0;
const int UPLOAD_INTERVAL = 3000;  // Send GPS data every 3 seconds

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(1000);

  Serial.println("=== Wio GPS WiFi Tracker ===");

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
  tft.setCursor(20, 40);
  tft.print("GPS WiFi Tracker");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 80);
  tft.print("Connecting to WiFi...");

  // Connect to WiFi
  connectWiFi();

  tft.setCursor(20, 100);
  tft.print("Waiting for GPS...");

  delay(2000);
  tft.fillScreen(TFT_BLACK);

  tripStartTime = millis();
}

void loop() {
  // Read GPS data
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Check buttons
  checkButtons();

  // Update display
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateGPSData();

    if (currentMode == SPEEDOMETER) {
      drawSpeedometerScreen();
    } else {
      drawStatsScreen();
    }
  }

  // Upload GPS data to server
  if (gpsValid && wifiConnected && (millis() - lastUpload >= UPLOAD_INTERVAL)) {
    uploadGPSData();
    lastUpload = millis();
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    tft.setTextColor(TFT_GREEN);
    tft.setCursor(20, 120);
    tft.print("WiFi OK: ");
    tft.print(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi failed!");

    tft.setTextColor(TFT_RED);
    tft.setCursor(20, 120);
    tft.print("WiFi FAILED");
  }
}

void uploadGPSData() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    return;
  }

  HTTPClient http;

  // Build URL with query parameters (easier for Wio Terminal)
  String url = String(serverURL);
  url += "?device_id=" + String(deviceId);
  url += "&lat=" + String(latitude, 6);
  url += "&lon=" + String(longitude, 6);
  url += "&speed=" + String(currentSpeed, 1);
  url += "&altitude=" + String(altitude, 1);
  url += "&satellites=" + String(satellites);

  Serial.println("Uploading: " + url);

  http.begin(url);
  http.setTimeout(5000);

  int httpCode = http.GET();

  if (httpCode == 200) {
    uploadCount++;
    Serial.println("Upload OK! Total: " + String(uploadCount));
  } else {
    uploadErrors++;
    Serial.println("Upload failed: " + String(httpCode));
  }

  http.end();
}

void checkButtons() {
  // TOP button - switch mode
  if (digitalRead(BUTTON_TOP) == LOW) {
    delay(200);
    currentMode = (currentMode == SPEEDOMETER) ? STATS : SPEEDOMETER;
    tft.fillScreen(TFT_BLACK);
    while (digitalRead(BUTTON_TOP) == LOW);
  }

  // MIDDLE button - reset peak
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    peakSpeed = 0.0;
    avgSpeed = 0.0;
    totalSpeed = 0.0;
    speedReadings = 0;
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // BOTTOM button - reset trip
  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    tripDistance = 0.0;
    tripStarted = false;
    tripStartTime = millis();
    peakSpeed = 0.0;
    avgSpeed = 0.0;
    totalSpeed = 0.0;
    speedReadings = 0;
    uploadCount = 0;
    uploadErrors = 0;
    while (digitalRead(BUTTON_BOT) == LOW);
  }
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

    // Calculate trip distance
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

  // Header
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("SPEEDOMETER");

  // WiFi & GPS status
  tft.setCursor(180, 5);
  if (wifiConnected) {
    tft.setTextColor(TFT_GREEN);
    tft.print("WiFi ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoWiFi ");
  }

  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoGPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);

  // Upload counter
  tft.setCursor(5, 15);
  tft.setTextColor(TFT_MAGENTA);
  tft.print("Uploads: ");
  tft.print(uploadCount);

  // Current speed - BIG
  tft.setTextSize(7);
  if (currentSpeed < 10) {
    tft.setCursor(100, 40);
  } else if (currentSpeed < 100) {
    tft.setCursor(70, 40);
  } else {
    tft.setCursor(40, 40);
  }

  if (currentSpeed < 30) {
    tft.setTextColor(TFT_GREEN);
  } else if (currentSpeed < 60) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }

  tft.print(currentSpeed, 0);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(230, 65);
  tft.print("MPH");

  // Altitude
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(200, 100);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Divider
  tft.drawLine(10, 120, 310, 120, TFT_DARKGREY);

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 130);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 155);
  tft.print("Avg:  ");
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Trip distance
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 180);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 205);
  tft.print("[A] Stats  [B] Reset Peak  [C] Reset All");

  // Server URL hint
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 220);
  tft.print("View map: ");
  tft.print(serverURL);
}

void drawStatsScreen() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("TRIP STATS");

  // WiFi & GPS status
  tft.setCursor(180, 5);
  if (wifiConnected) {
    tft.setTextColor(TFT_GREEN);
    tft.print("WiFi ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoWiFi ");
  }

  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoGPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(60, 25);
  tft.print("TRIP SUMMARY");

  tft.drawLine(10, 50, 310, 50, TFT_DARKGREY);

  // Trip duration
  unsigned long tripDuration = millis() - tripStartTime;
  int hours = tripDuration / 3600000;
  int minutes = (tripDuration % 3600000) / 60000;
  int seconds = (tripDuration % 60000) / 1000;

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 60);
  tft.print("Duration: ");
  tft.setTextColor(TFT_GREEN);
  if (hours > 0) {
    tft.print(hours);
    tft.print("h ");
  }
  tft.print(minutes);
  tft.print("m ");
  tft.print(seconds);
  tft.print("s");

  // Distance
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 85);
  tft.print("Distance: ");
  tft.setTextColor(TFT_MAGENTA);
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // Speeds
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 110);
  tft.print("Current:  ");
  tft.setTextColor(TFT_GREEN);
  tft.print(currentSpeed, 1);
  tft.print(" MPH");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 135);
  tft.print("Peak:     ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 160);
  tft.print("Average:  ");
  tft.setTextColor(TFT_CYAN);
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Upload stats
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 185);
  tft.print("Uploads:  ");
  tft.setTextColor(TFT_MAGENTA);
  tft.print(uploadCount);
  tft.setTextColor(TFT_WHITE);
  tft.print(" (");
  tft.setTextColor(TFT_RED);
  tft.print(uploadErrors);
  tft.setTextColor(TFT_WHITE);
  tft.print(" err)");

  // Coordinates
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 210);
    tft.print("LAT: ");
    tft.print(latitude, 6);
    tft.print("  LON: ");
    tft.print(longitude, 6);
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 225);
  tft.print("[A] Speed  [B] Reset Peak  [C] Reset All");
}
