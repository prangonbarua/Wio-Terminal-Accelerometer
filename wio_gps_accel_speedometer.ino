// GPS + Accelerometer Speedometer for Wio Terminal
// Uses NEO-6M GPS on Serial1 (Pins 8/10) + Built-in Accelerometer
// Displays both GPS speed and G-force

#include <LIS3DHTR.h>
#include "TFT_eSPI.h"
#include <TinyGPS++.h>

LIS3DHTR<TwoWire> lis;
TFT_eSPI tft;
TinyGPSPlus gps;

// Speed tracking (MPH)
float gpsSpeed = 0.0;
float peakSpeed = 0.0;

// G-force tracking
float gForce = 0.0;
float peakG = 0.0;

// GPS data
int satellites = 0;
double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;
bool gpsValid = false;

// Accelerometer calibration
float offsetX = 0.0;
float offsetY = 0.0;
float offsetZ = 0.0;

// Button pins
const int BUTTON_TOP = WIO_KEY_A;    // Switch display mode
const int BUTTON_MID = WIO_KEY_B;    // Reset peaks
const int BUTTON_BOT = WIO_KEY_C;    // Recalibrate

// Display mode
int displayMode = 0;  // 0=Speed, 1=G-Force, 2=Both

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 100;  // 10Hz

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);  // GPS on Serial1
  delay(1000);

  Serial.println("=== GPS + Accelerometer Speedometer ===");

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
  tft.print("GPS + G-FORCE");
  tft.setCursor(20, 70);
  tft.print("SPEEDOMETER");

  // Initialize accelerometer
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 110);
  tft.print("Initializing accelerometer...");

  lis.begin(Wire1);

  if (!lis) {
    Serial.println("Accelerometer not found!");
    tft.setTextColor(TFT_RED);
    tft.setCursor(20, 125);
    tft.print("Accelerometer: FAILED");
  } else {
    Serial.println("Accelerometer OK!");
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(20, 125);
    tft.print("Accelerometer: OK");

    lis.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
    lis.setFullScaleRange(LIS3DHTR_RANGE_4G);
  }

  // Calibrate accelerometer
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 145);
  tft.print("Calibrating - keep still...");

  calibrateAccel();

  tft.setTextColor(TFT_GREEN);
  tft.setCursor(20, 160);
  tft.print("Calibration complete!");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 180);
  tft.print("Waiting for GPS...");

  delay(2000);
  tft.fillScreen(TFT_BLACK);

  Serial.println("Ready! Waiting for GPS...");
}

void loop() {
  // Read GPS data
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    Serial.print(c);  // Debug: show raw GPS data
    gps.encode(c);
  }

  // Check buttons
  checkButtons();

  // Update at set interval
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    updateGPSData();
    updateAccelData();
    drawScreen();
  }
}

void checkButtons() {
  // TOP button - switch display mode
  if (digitalRead(BUTTON_TOP) == LOW) {
    delay(200);
    displayMode = (displayMode + 1) % 3;
    tft.fillScreen(TFT_BLACK);
    while (digitalRead(BUTTON_TOP) == LOW);
  }

  // MIDDLE button - reset peaks
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    peakSpeed = 0.0;
    peakG = 0.0;
    Serial.println("Peaks reset");
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // BOTTOM button - recalibrate
  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(40, 100);
    tft.print("Calibrating...");
    calibrateAccel();
    tft.fillScreen(TFT_BLACK);
    while (digitalRead(BUTTON_BOT) == LOW);
  }
}

void calibrateAccel() {
  float sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < 100; i++) {
    sumX += lis.getAccelerationX();
    sumY += lis.getAccelerationY();
    sumZ += lis.getAccelerationZ();
    delay(10);
  }

  offsetX = sumX / 100.0;
  offsetY = sumY / 100.0;
  offsetZ = sumZ / 100.0 - 1.0;  // Account for gravity

  Serial.println("Accelerometer calibrated");
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
      gpsSpeed = gps.speed.mph();
      if (gpsSpeed > peakSpeed) {
        peakSpeed = gpsSpeed;
      }
    }
  } else {
    gpsValid = false;
  }

  satellites = gps.satellites.value();
}

void updateAccelData() {
  if (!lis) return;

  float ax = lis.getAccelerationX() - offsetX;
  float ay = lis.getAccelerationY() - offsetY;
  float az = lis.getAccelerationZ() - offsetZ - 1.0;

  // Calculate total G-force
  gForce = sqrt(ax*ax + ay*ay + az*az);

  // Filter noise
  if (gForce < 0.05) {
    gForce = 0.0;
  }

  // Update peak
  if (gForce > peakG) {
    peakG = gForce;
  }
}

void drawScreen() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextSize(1);
  tft.setCursor(5, 5);

  // GPS status
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS LOCKED ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

  // Mode indicator
  tft.setCursor(200, 5);
  tft.setTextColor(TFT_MAGENTA);
  if (displayMode == 0) tft.print("[SPEED]");
  else if (displayMode == 1) tft.print("[G-FORCE]");
  else tft.print("[BOTH]");

  if (displayMode == 0) {
    drawSpeedScreen();
  } else if (displayMode == 1) {
    drawGForceScreen();
  } else {
    drawCombinedScreen();
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 220);
  tft.print("[A] Mode  [B] Reset  [C] Calibrate");
}

void drawSpeedScreen() {
  // Big speed display
  tft.setTextSize(7);
  int speedX = 70;
  if (gpsSpeed >= 100) speedX = 30;
  else if (gpsSpeed >= 10) speedX = 50;
  tft.setCursor(speedX, 40);

  // Color based on speed
  if (gpsSpeed < 30) {
    tft.setTextColor(TFT_GREEN);
  } else if (gpsSpeed < 60) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(gpsSpeed, 0);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(230, 60);
  tft.print("MPH");

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 120);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Altitude
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(10, 150);
  tft.print("Alt: ");
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // G-Force (small)
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 180);
  tft.print("G-Force: ");
  tft.print(gForce, 2);
  tft.print(" G");
}

void drawGForceScreen() {
  // Big G-force display
  tft.setTextSize(6);
  tft.setCursor(60, 40);

  // Color based on G
  if (gForce < 1.0) {
    tft.setTextColor(TFT_GREEN);
  } else if (gForce < 2.0) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(gForce, 2);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(240, 60);
  tft.print("G");

  // Peak G
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 120);
  tft.print("Peak G: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakG, 2);
  tft.print(" G");

  // Speed (small)
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 150);
  tft.print("Speed: ");
  tft.print(gpsSpeed, 1);
  tft.print(" MPH");

  // G-force bar
  tft.drawRect(10, 180, 300, 25, TFT_WHITE);
  int barWidth = min((int)(gForce * 75), 296);
  uint16_t barColor = (gForce < 1.0) ? TFT_GREEN : (gForce < 2.0) ? TFT_YELLOW : TFT_RED;
  tft.fillRect(12, 182, barWidth, 21, barColor);
}

void drawCombinedScreen() {
  // Speed (left side)
  tft.setTextSize(5);
  tft.setCursor(10, 30);

  if (gpsSpeed < 30) {
    tft.setTextColor(TFT_GREEN);
  } else if (gpsSpeed < 60) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(gpsSpeed, 0);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 80);
  tft.print("MPH");

  // G-Force (right side)
  tft.setTextSize(5);
  tft.setCursor(180, 30);

  if (gForce < 1.0) {
    tft.setTextColor(TFT_GREEN);
  } else if (gForce < 2.0) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(gForce, 1);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(260, 80);
  tft.print("G");

  // Divider
  tft.drawLine(160, 25, 160, 100, TFT_DARKGREY);

  // Peaks
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 115);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 0);
  tft.print(" MPH");

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(180, 115);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakG, 1);
  tft.print(" G");

  // Altitude
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(10, 145);
  tft.print("Alt: ");
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Coordinates
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 175);
    tft.print("LAT: ");
    tft.print(latitude, 5);
    tft.setCursor(10, 190);
    tft.print("LON: ");
    tft.print(longitude, 5);
  }
}
