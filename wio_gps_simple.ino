// Simple GPS Speedometer for Wio Terminal
// No SD card required - just displays speed on screen
// NEO-6M GPS on Serial1 (back header pins 8/10)

#include "TFT_eSPI.h"
#include <TinyGPS++.h>

TFT_eSPI tft;
TinyGPSPlus gps;

// Speed tracking (MPH)
float currentSpeed = 0.0;
float peakSpeed = 0.0;

// GPS data
int satellites = 0;
double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;
bool gpsValid = false;

// Button pins
const int BUTTON_MID = WIO_KEY_B;    // Reset peak

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);  // GPS on Serial1
  delay(1000);

  Serial.println("=== Simple GPS Speedometer ===");

  // Setup button
  pinMode(BUTTON_MID, INPUT_PULLUP);

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
  tft.setCursor(40, 80);
  tft.print("GPS SPEEDOMETER");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(40, 120);
  tft.print("Waiting for GPS signal...");

  delay(2000);
  tft.fillScreen(TFT_BLACK);

  Serial.println("Waiting for GPS...");
}

void loop() {
  // Read GPS data and print raw data for debugging
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    Serial.print(c);  // Print raw GPS data to Serial Monitor
    gps.encode(c);
  }

  // Check button - reset peak
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    peakSpeed = 0.0;
    Serial.println("Peak reset");
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // Update display
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateGPSData();
    drawScreen();
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
    }
  } else {
    gpsValid = false;
  }

  satellites = gps.satellites.value();
}

void drawScreen() {
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

  // Current speed - BIG
  tft.setTextSize(7);
  int speedX = 100;
  if (currentSpeed >= 100) speedX = 40;
  else if (currentSpeed >= 10) speedX = 70;
  tft.setCursor(speedX, 40);

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

  // Altitude
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(180, 95);
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

  // Coordinates
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 160);
    tft.print("LAT: ");
    tft.print(latitude, 6);
    tft.setCursor(10, 175);
    tft.print("LON: ");
    tft.print(longitude, 6);
  }

  // Button hint
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 210);
  tft.print("[B] Reset Peak Speed");
}
