#include "TFT_eSPI.h"
#include <TinyGPS++.h>

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
bool gpsValid = false;

// Trip data
float tripDistance = 0.0; // miles
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;

// Button pins
const int BUTTON_MID = WIO_KEY_B;    // Middle button - reset peak
const int BUTTON_BOT = WIO_KEY_C;    // Bottom button - reset trip

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200; // 5Hz update rate

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); // GPS on Serial1
  delay(1000);

  Serial.println("=== Wio Terminal GPS Speedometer ===");

  // Setup buttons
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
  tft.setTextSize(3);
  tft.setCursor(20, 80);
  tft.print("GPS SPEEDOMETER");
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 130);
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

void updateGPSData() {
  if (gps.location.isValid()) {
    gpsValid = true;
    latitude = gps.location.lat();
    longitude = gps.location.lng();

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
  tft.print(" SATELLITES");

  // Current speed - BIG
  tft.setTextSize(7);
  if (currentSpeed < 10) {
    tft.setCursor(100, 35);
  } else if (currentSpeed < 100) {
    tft.setCursor(70, 35);
  } else {
    tft.setCursor(40, 35);
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
  tft.setCursor(230, 60);
  tft.print("MPH");

  // Divider line
  tft.drawLine(10, 105, 310, 105, TFT_DARKGREY);

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 115);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average speed
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 140);
  tft.print("Avg:  ");
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Trip distance
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 165);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // GPS coordinates (small)
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(180, 115);
    tft.print("LAT: ");
    tft.print(latitude, 5);
    tft.setCursor(180, 127);
    tft.print("LON: ");
    tft.print(longitude, 5);
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 220);
  tft.print("[B] Reset Peak/Avg    [C] Reset Trip");
}
