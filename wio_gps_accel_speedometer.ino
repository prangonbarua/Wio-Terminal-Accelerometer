/*
 * GPS + Accelerometer Speedometer for Wio Terminal
 *
 * WIRING (NEO-6M GPS):
 *   GPS VCC  ->  Wio 3.3V or 5V
 *   GPS GND  ->  Wio GND
 *   GPS TXD  ->  Wio Pin 8
 *   GPS RXD  ->  Wio Pin 10 (optional)
 */

#include <LIS3DHTR.h>
#include "TFT_eSPI.h"
#include <TinyGPS++.h>

LIS3DHTR<TwoWire> lis;
TFT_eSPI tft;
TinyGPSPlus gps;

// GPS tracking
float gpsSpeed = 0.0;
float peakSpeed = 0.0;
int satellites = 0;
double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;
bool gpsValid = false;
unsigned long gpsCharsReceived = 0;

// G-force tracking
float gForce = 0.0;
float peakG = 0.0;
float offsetX = 0, offsetY = 0, offsetZ = 0;

// Buttons
const int BTN_A = WIO_KEY_A;
const int BTN_B = WIO_KEY_B;
const int BTN_C = WIO_KEY_C;

// Display
int displayMode = 0;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);  // GPS baud rate

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_C, INPUT_PULLUP);

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Splash
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(40, 60);
  tft.print("GPS SPEEDOMETER");

  // Init accelerometer
  lis.begin(Wire1);
  if (lis) {
    lis.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
    lis.setFullScaleRange(LIS3DHTR_RANGE_4G);
    calibrateAccel();
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(1);
    tft.setCursor(40, 100);
    tft.print("Accelerometer: OK");
  } else {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.setCursor(40, 100);
    tft.print("Accelerometer: FAILED");
  }

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(40, 120);
  tft.print("Waiting for GPS...");

  delay(2000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // Read GPS data
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    gpsCharsReceived++;
    Serial.print(c);  // Print to Serial Monitor for debugging
    gps.encode(c);
  }

  // Buttons
  if (digitalRead(BTN_A) == LOW) {
    delay(200);
    displayMode = (displayMode + 1) % 3;
    while (digitalRead(BTN_A) == LOW);
  }

  if (digitalRead(BTN_B) == LOW) {
    delay(200);
    peakSpeed = 0;
    peakG = 0;
    while (digitalRead(BTN_B) == LOW);
  }

  if (digitalRead(BTN_C) == LOW) {
    delay(200);
    calibrateAccel();
    while (digitalRead(BTN_C) == LOW);
  }

  // Update display at 10Hz
  if (millis() - lastUpdate >= 100) {
    lastUpdate = millis();

    // Update GPS data
    if (gps.location.isValid()) {
      gpsValid = true;
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      if (gps.altitude.isValid()) altitude = gps.altitude.meters();
      if (gps.speed.isValid()) {
        gpsSpeed = gps.speed.mph();
        if (gpsSpeed > peakSpeed) peakSpeed = gpsSpeed;
      }
    } else {
      gpsValid = false;
    }
    satellites = gps.satellites.value();

    // Update accelerometer
    if (lis) {
      float ax = lis.getAccelerationX() - offsetX;
      float ay = lis.getAccelerationY() - offsetY;
      float az = lis.getAccelerationZ() - offsetZ - 1.0;
      gForce = sqrt(ax*ax + ay*ay + az*az);
      if (gForce < 0.05) gForce = 0;
      if (gForce > peakG) peakG = gForce;
    }

    drawScreen();
  }
}

void calibrateAccel() {
  if (!lis) return;
  float sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 50; i++) {
    sx += lis.getAccelerationX();
    sy += lis.getAccelerationY();
    sz += lis.getAccelerationZ();
    delay(10);
  }
  offsetX = sx / 50.0;
  offsetY = sy / 50.0;
  offsetZ = sz / 50.0 - 1.0;
}

void drawScreen() {
  tft.fillScreen(TFT_BLACK);

  // Top status bar
  tft.setTextSize(1);
  tft.setCursor(5, 5);

  // GPS status with data indicator
  if (gpsCharsReceived == 0) {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS DATA");
  } else if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS OK ");
    tft.print(satellites);
    tft.print(" SAT");
  } else {
    tft.setTextColor(TFT_YELLOW);
    tft.print("GPS WAIT ");
    tft.print(satellites);
    tft.print(" SAT");
  }

  // Show chars received (helps debug)
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(200, 5);
  tft.print(gpsCharsReceived);
  tft.print(" bytes");

  // Main display based on mode
  if (displayMode == 0) {
    drawSpeedMode();
  } else if (displayMode == 1) {
    drawGForceMode();
  } else {
    drawBothMode();
  }

  // Bottom buttons
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 225);
  tft.print("[A]Mode [B]Reset [C]Calibrate");
}

void drawSpeedMode() {
  // Big speed
  tft.setTextSize(8);
  tft.setCursor(50, 50);

  if (gpsSpeed < 30) tft.setTextColor(TFT_GREEN);
  else if (gpsSpeed < 60) tft.setTextColor(TFT_YELLOW);
  else tft.setTextColor(TFT_RED);

  tft.print((int)gpsSpeed);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(230, 70);
  tft.print("MPH");

  // Stats
  tft.setTextSize(2);

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 130);
  tft.print("Peak: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(10, 160);
  tft.print("Alt: ");
  tft.print((int)(altitude * 3.28084));
  tft.print(" ft");

  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 190);
  tft.print("G: ");
  tft.print(gForce, 2);
}

void drawGForceMode() {
  // Big G
  tft.setTextSize(7);
  tft.setCursor(70, 50);

  if (gForce < 1.0) tft.setTextColor(TFT_GREEN);
  else if (gForce < 2.0) tft.setTextColor(TFT_YELLOW);
  else tft.setTextColor(TFT_RED);

  tft.print(gForce, 1);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(250, 70);
  tft.print("G");

  // G bar
  tft.drawRect(10, 130, 300, 30, TFT_WHITE);
  int barW = min((int)(gForce * 75), 296);
  uint16_t col = (gForce < 1.0) ? TFT_GREEN : (gForce < 2.0) ? TFT_YELLOW : TFT_RED;
  if (barW > 0) tft.fillRect(12, 132, barW, 26, col);

  // Stats
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 175);
  tft.print("Peak G: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(peakG, 2);

  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 200);
  tft.print("Speed: ");
  tft.print(gpsSpeed, 0);
  tft.print(" MPH");
}

void drawBothMode() {
  // Speed left
  tft.setTextSize(5);
  tft.setCursor(10, 40);
  if (gpsSpeed < 30) tft.setTextColor(TFT_GREEN);
  else if (gpsSpeed < 60) tft.setTextColor(TFT_YELLOW);
  else tft.setTextColor(TFT_RED);
  tft.print((int)gpsSpeed);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 90);
  tft.print("MPH");

  // G right
  tft.setTextSize(5);
  tft.setCursor(180, 40);
  if (gForce < 1.0) tft.setTextColor(TFT_GREEN);
  else if (gForce < 2.0) tft.setTextColor(TFT_YELLOW);
  else tft.setTextColor(TFT_RED);
  tft.print(gForce, 1);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(270, 90);
  tft.print("G");

  // Divider
  tft.drawLine(160, 30, 160, 110, TFT_DARKGREY);

  // Peaks
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 125);
  tft.print("Pk:");
  tft.setTextColor(TFT_WHITE);
  tft.print(peakSpeed, 0);

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(180, 125);
  tft.print("Pk:");
  tft.setTextColor(TFT_WHITE);
  tft.print(peakG, 1);

  // Alt & coords
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(10, 155);
  tft.print("Alt: ");
  tft.print((int)(altitude * 3.28084));
  tft.print(" ft");

  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 185);
    tft.print("LAT:");
    tft.print(latitude, 5);
    tft.setCursor(10, 200);
    tft.print("LON:");
    tft.print(longitude, 5);
  }
}
