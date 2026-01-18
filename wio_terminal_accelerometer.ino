#include <LIS3DHTR.h>
#include "TFT_eSPI.h"

LIS3DHTR<TwoWire> lis;
TFT_eSPI tft;

// Speed tracking (MPH)
float currentSpeed = 0.0;
float peakSpeed = 0.0;

// Calibration
float offsetX = 0.0;
float offsetY = 0.0;
float offsetZ = 0.0;

// Smoothing
const int numReadings = 10;
float readings[numReadings];
int readIndex = 0;
float total = 0.0;

const float NOISE_THRESHOLD = 0.5;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Wio Terminal Speedometer ===");

  // Turn on backlight
  pinMode(72, OUTPUT);
  digitalWrite(72, HIGH);
  Serial.println("Backlight ON");

  // Initialize display
  tft.init();
  tft.setRotation(3);
  Serial.println("Display initialized");

  // Initialize accelerometer
  lis.begin(Wire1);

  if (!lis) {
    Serial.println("ERROR: Accelerometer not found!");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(3);
    tft.setCursor(20, 100);
    tft.print("SENSOR ERROR!");
    while (1) delay(1000);
  }

  Serial.println("Accelerometer OK!");

  lis.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G);

  // Calibration
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("Calibrating...");
  tft.setCursor(20, 100);
  tft.print("Keep still!");

  Serial.println("Calibrating...");
  delay(1500);

  calibrate();

  // Initialize smoothing
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0.0;
  }

  // Ready
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(4);
  tft.setCursor(60, 100);
  tft.print("READY!");

  Serial.println("READY!");
  delay(1500);
}

void loop() {
  // Read accelerometer
  float ax = lis.getAccelerationX() - offsetX;
  float ay = lis.getAccelerationY() - offsetY;
  float az = lis.getAccelerationZ() - offsetZ - 1.0;

  float mag = sqrt(ax*ax + ay*ay + az*az);
  float accel = mag * 9.81; // m/s²

  if (accel < NOISE_THRESHOLD) {
    accel = 0.0;
  }

  // Smooth
  total = total - readings[readIndex];
  readings[readIndex] = accel;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  float currentAccel = total / numReadings;

  // Convert m/s² to MPH (approximate instantaneous speed change)
  // MPH = m/s² * 2.23694 (converting m/s to MPH)
  currentSpeed = currentAccel * 2.23694;

  // Update peak
  if (currentSpeed > peakSpeed) {
    peakSpeed = currentSpeed;
  }

  // Display
  drawScreen();

  // Serial
  Serial.print("Current: ");
  Serial.print(currentSpeed, 2);
  Serial.print(" MPH | Peak: ");
  Serial.print(peakSpeed, 2);
  Serial.println(" MPH");

  delay(100);
}

void calibrate() {
  float sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < 100; i++) {
    sumX += lis.getAccelerationX();
    sumY += lis.getAccelerationY();
    sumZ += lis.getAccelerationZ();
    delay(10);
  }

  offsetX = sumX / 100.0;
  offsetY = sumY / 100.0;
  offsetZ = sumZ / 100.0 - 1.0;

  Serial.println("Calibration complete");
}

void drawScreen() {
  tft.fillScreen(TFT_BLACK);

  // Current Speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 30);
  tft.print("Current:");

  tft.setTextSize(4);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 60);
  tft.print(currentSpeed, 1);
  tft.setTextSize(2);
  tft.print(" MPH");

  // Peak Speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 120);
  tft.print("Peak:");

  tft.setTextSize(4);
  tft.setTextColor(TFT_RED);
  tft.setCursor(10, 150);
  tft.print(peakSpeed, 1);
  tft.setTextSize(2);
  tft.print(" MPH");

  // Info
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 220);
  tft.print("Press RESET to clear peak");
}
