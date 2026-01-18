#include <LIS3DHTR.h>
#include "TFT_eSPI.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

LIS3DHTR<TwoWire> lis;
TFT_eSPI tft;

// WiFi credentials - UPDATE THESE!
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Mode tracking
enum Mode { SPEEDOMETER, BILLBOARD };
Mode currentMode = SPEEDOMETER;

// Speed tracking
float currentSpeed = 0.0;
float peakSpeed = 0.0;

// Calibration
float offsetX = 0.0;
float offsetY = 0.0;
float offsetZ = 0.0;
float speedCalibration = 1.0;

// Smoothing
const int numReadings = 10;
float readings[numReadings];
int readIndex = 0;
float total = 0.0;

const float NOISE_THRESHOLD = 0.5;

// Billboard data
String currentSong = "Loading...";
String currentArtist = "";
String albumCoverURL = "";
bool billboardDataLoaded = false;

// Button pin (top button on Wio Terminal)
const int BUTTON_1 = WIO_KEY_A; // Top button

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Wio Terminal Multi-Mode ===");

  // Setup button
  pinMode(BUTTON_1, INPUT_PULLUP);

  // Turn on backlight
  pinMode(72, OUTPUT);
  digitalWrite(72, HIGH);

  // Initialize display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Show splash screen
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(3);
  tft.setCursor(20, 80);
  tft.print("MULTI MODE");
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 120);
  tft.print("Speedometer");
  tft.setCursor(20, 145);
  tft.print("Billboard Hot 100");
  delay(2000);

  // Initialize accelerometer
  lis.begin(Wire1);

  if (!lis) {
    Serial.println("ERROR: Accelerometer not found!");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.print("SENSOR ERROR!");
    while (1) delay(1000);
  }

  lis.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G);

  // Zero calibration
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("Calibrating...");
  tft.setCursor(20, 100);
  tft.print("Keep still!");

  delay(1500);
  calibrateZero();

  // Initialize smoothing
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0.0;
  }

  // Connect to WiFi for Billboard data
  connectWiFi();

  // Fetch Billboard Hot 100 data
  fetchBillboardData();

  // Ready
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.setCursor(60, 100);
  tft.print("READY!");
  delay(1500);
}

void loop() {
  // Check for button press to switch modes
  if (digitalRead(BUTTON_1) == LOW) {
    delay(200); // Debounce
    switchMode();
    delay(500); // Prevent multiple triggers
  }

  // Update based on current mode
  if (currentMode == SPEEDOMETER) {
    updateSpeedometer();
  } else if (currentMode == BILLBOARD) {
    displayBillboard();
  }

  delay(100);
}

void switchMode() {
  if (currentMode == SPEEDOMETER) {
    currentMode = BILLBOARD;
    Serial.println("Switched to Billboard Mode");
  } else {
    currentMode = SPEEDOMETER;
    Serial.println("Switched to Speedometer Mode");
  }
  tft.fillScreen(TFT_BLACK);
}

void updateSpeedometer() {
  // Read accelerometer
  float ax = lis.getAccelerationX() - offsetX;
  float ay = lis.getAccelerationY() - offsetY;
  float az = lis.getAccelerationZ() - offsetZ - 1.0;

  float mag = sqrt(ax*ax + ay*ay + az*az);
  float accel = mag * 9.81;

  if (accel < NOISE_THRESHOLD) {
    accel = 0.0;
  }

  // Smooth
  total = total - readings[readIndex];
  readings[readIndex] = accel;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  float currentAccel = total / numReadings;
  currentSpeed = currentAccel * 2.23694 * speedCalibration;

  if (currentSpeed > peakSpeed) {
    peakSpeed = currentSpeed;
  }

  drawSpeedometerScreen();
}

void drawSpeedometerScreen() {
  tft.fillScreen(TFT_BLACK);

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("MODE: SPEEDOMETER");

  // Current Speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 40);
  tft.print("Current:");

  tft.setTextSize(4);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 70);
  tft.print(currentSpeed, 1);
  tft.setTextSize(2);
  tft.print(" MPH");

  // Peak Speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 130);
  tft.print("Peak:");

  tft.setTextSize(4);
  tft.setTextColor(TFT_RED);
  tft.setCursor(10, 160);
  tft.print(peakSpeed, 1);
  tft.setTextSize(2);
  tft.print(" MPH");

  // Button hint
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 220);
  tft.print("Press TOP button for Billboard");
}

void displayBillboard() {
  tft.fillScreen(TFT_BLACK);

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("MODE: BILLBOARD HOT 100");

  // Title
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 30);
  tft.print("#1 This Week:");

  if (billboardDataLoaded) {
    // Song name
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 65);

    // Word wrap for song title
    if (currentSong.length() > 20) {
      tft.print(currentSong.substring(0, 20));
      tft.setCursor(10, 90);
      tft.print(currentSong.substring(20));
    } else {
      tft.print(currentSong);
    }

    // Artist name
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 125);
    tft.print("by ");

    if (currentArtist.length() > 18) {
      tft.print(currentArtist.substring(0, 18));
      tft.setCursor(10, 150);
      tft.print(currentArtist.substring(18));
    } else {
      tft.print(currentArtist);
    }

    // Album cover placeholder (if you want to download and display image)
    tft.drawRect(200, 60, 100, 100, TFT_WHITE);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(210, 105);
    tft.print("ALBUM");
    tft.setCursor(210, 115);
    tft.print("COVER");

  } else {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 80);
    tft.print("No WiFi Data");
  }

  // Button hint
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 220);
  tft.print("Press TOP button for Speed");
}

void calibrateZero() {
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

  Serial.println("Zero calibration complete");
}

void connectWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("Connecting WiFi...");

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    tft.setCursor(20, 100);
    tft.setTextColor(TFT_GREEN);
    tft.print("WiFi OK!");
    delay(1000);
  } else {
    Serial.println("\nWiFi failed!");
    tft.setCursor(20, 100);
    tft.setTextColor(TFT_RED);
    tft.print("WiFi Failed");
    delay(2000);
  }
}

void fetchBillboardData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi - skipping Billboard fetch");
    billboardDataLoaded = false;
    return;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("Fetching");
  tft.setCursor(20, 90);
  tft.print("Billboard Data...");

  HTTPClient http;

  // Use your Flask app URL or a simplified API endpoint
  // For now, we'll use a placeholder - you'll need to create an API endpoint
  String url = "http://YOUR_FLASK_APP_URL/api/hot100/current";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();

    // Parse JSON response
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    currentSong = doc["song"].as<String>();
    currentArtist = doc["artist"].as<String>();
    albumCoverURL = doc["image_url"].as<String>();

    billboardDataLoaded = true;

    Serial.println("Billboard data loaded:");
    Serial.println("Song: " + currentSong);
    Serial.println("Artist: " + currentArtist);

    tft.setCursor(20, 130);
    tft.setTextColor(TFT_GREEN);
    tft.print("Data Loaded!");
    delay(1000);
  } else {
    Serial.println("HTTP request failed");
    billboardDataLoaded = false;

    // Fallback: Use hardcoded latest #1 or read from SD card
    currentSong = "Loading failed";
    currentArtist = "Check WiFi";
  }

  http.end();
}
