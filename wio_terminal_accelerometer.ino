#include <LIS3DHTR.h>
#include <TFT_eSPI.h>

LIS3DHTR<TwoWire> lis;
TFT_eSPI tft;

// Acceleration tracking variables
float currentAccel = 0.0;
float peakAccel = 0.0;

// Calibration offsets
float offsetX = 0.0;
float offsetY = 0.0;
float offsetZ = 0.0;

// Smoothing filter
const int numReadings = 10;
float readings[numReadings];
int readIndex = 0;
float total = 0.0;

// Noise threshold (m/s²)
const float NOISE_THRESHOLD = 0.5;

void setup() {
  Serial.begin(115200);

  // Initialize LCD
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);

  // Initialize accelerometer
  lis.begin(Wire1);

  if (!lis) {
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED);
    tft.println("ERROR:");
    tft.setCursor(10, 40);
    tft.println("Accelerometer");
    tft.setCursor(10, 70);
    tft.println("not found!");
    while (1) {
      delay(1000);
    }
  }

  // Configure accelerometer
  lis.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G);

  // Display calibration instructions
  tft.setCursor(10, 10);
  tft.setTextColor(TFT_CYAN);
  tft.println("Calibrating...");
  tft.setCursor(10, 40);
  tft.setTextColor(TFT_YELLOW);
  tft.println("Keep device still");
  tft.setCursor(10, 70);
  tft.println("on flat surface");

  // Calibrate - measure offset when stationary
  delay(1500);
  calibrateAccelerometer();

  // Initialize smoothing array
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0.0;
  }

  // Clear screen and show ready message
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(80, 100);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.println("READY!");
  delay(1000);
  tft.fillScreen(TFT_BLACK);

  Serial.println("Accelerometer initialized and calibrated");
}

void loop() {
  // Read raw acceleration values (in g's)
  float accelX = lis.getAccelerationX() - offsetX;
  float accelY = lis.getAccelerationY() - offsetY;
  float accelZ = lis.getAccelerationZ() - offsetZ - 1.0; // Remove gravity component

  // Calculate total acceleration magnitude
  float accelMagnitude = sqrt(accelX * accelX +
                               accelY * accelY +
                               accelZ * accelZ);

  // Convert from g's to m/s²
  float accelMS2 = accelMagnitude * 9.81;

  // Apply noise threshold filter
  if (accelMS2 < NOISE_THRESHOLD) {
    accelMS2 = 0.0;
  }

  // Apply smoothing filter
  total = total - readings[readIndex];
  readings[readIndex] = accelMS2;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  currentAccel = total / numReadings;

  // Update peak acceleration
  if (currentAccel > peakAccel) {
    peakAccel = currentAccel;
  }

  // Update display
  updateDisplay();

  // Print to Serial Monitor
  Serial.print("Current: ");
  Serial.print(currentAccel, 2);
  Serial.print(" m/s² | Peak: ");
  Serial.print(peakAccel, 2);
  Serial.println(" m/s²");

  delay(50); // Update rate: 20Hz
}

void calibrateAccelerometer() {
  float sumX = 0.0, sumY = 0.0, sumZ = 0.0;
  const int calibrationSamples = 100;

  for (int i = 0; i < calibrationSamples; i++) {
    sumX += lis.getAccelerationX();
    sumY += lis.getAccelerationY();
    sumZ += lis.getAccelerationZ();
    delay(10);
  }

  offsetX = sumX / calibrationSamples;
  offsetY = sumY / calibrationSamples;
  offsetZ = sumZ / calibrationSamples - 1.0; // Subtract 1g for gravity

  Serial.println("Calibration complete:");
  Serial.print("  Offset X: "); Serial.println(offsetX, 4);
  Serial.print("  Offset Y: "); Serial.println(offsetY, 4);
  Serial.print("  Offset Z: "); Serial.println(offsetZ, 4);
}

void updateDisplay() {
  tft.fillScreen(TFT_BLACK);

  // Current acceleration section
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 20);
  tft.println("Current Accel:");

  tft.setTextSize(4);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 50);
  tft.print(currentAccel, 2);

  tft.setTextSize(2);
  tft.setCursor(180, 70);
  tft.println("m/s");
  tft.setCursor(240, 65);
  tft.setTextSize(1);
  tft.println("2");

  // Peak acceleration section
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 110);
  tft.println("Peak Accel:");

  tft.setTextSize(4);
  tft.setTextColor(TFT_RED);
  tft.setCursor(10, 140);
  tft.print(peakAccel, 2);

  tft.setTextSize(2);
  tft.setCursor(180, 160);
  tft.println("m/s");
  tft.setCursor(240, 155);
  tft.setTextSize(1);
  tft.println("2");

  // Instructions
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 210);
  tft.println("Press RESET button to clear peak");
}
