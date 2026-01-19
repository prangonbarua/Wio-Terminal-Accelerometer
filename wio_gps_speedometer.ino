#include <LIS3DHTR.h>
#include "TFT_eSPI.h"
#include <TinyGPS++.h>

LIS3DHTR<TwoWire> lis;
TFT_eSPI tft;
TinyGPSPlus gps;

// Mode tracking
enum Mode { SPEEDOMETER, BILLBOARD };
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
bool gpsValid = false;

// Trip data
float tripDistance = 0.0; // miles
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;

// Billboard data (hardcoded until WiFi module)
String currentSong = "APT.";
String currentArtist = "ROSE & Bruno Mars";

// Button pins
const int BUTTON_TOP = WIO_KEY_A;    // Top button - switch modes
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
  tft.setTextSize(3);
  tft.setCursor(30, 60);
  tft.print("GPS SPEEDO");
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 100);
  tft.print("+ Billboard Hot 100");
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 140);
  tft.print("Waiting for GPS signal...");
  delay(2000);

  // Initialize accelerometer (backup)
  lis.begin(Wire1);
  if (lis) {
    lis.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
    lis.setFullScaleRange(LIS3DHTR_RANGE_2G);
    Serial.println("Accelerometer ready (backup)");
  }

  Serial.println("Waiting for GPS...");
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

    // Display based on mode
    if (currentMode == SPEEDOMETER) {
      drawSpeedometerScreen();
    } else {
      drawBillboardScreen();
    }
  }
}

void checkButtons() {
  // TOP button - switch modes
  if (digitalRead(BUTTON_TOP) == LOW) {
    delay(200);
    switchMode();
    while (digitalRead(BUTTON_TOP) == LOW); // Wait for release
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

void switchMode() {
  if (currentMode == SPEEDOMETER) {
    currentMode = BILLBOARD;
    Serial.println("Mode: Billboard Hot 100");
  } else {
    currentMode = SPEEDOMETER;
    Serial.println("Mode: Speedometer");
  }
  tft.fillScreen(TFT_BLACK);
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

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("GPS SPEEDOMETER");

  // GPS status
  tft.setCursor(220, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS OK ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

  // Current speed - BIG
  tft.setTextSize(6);
  if (currentSpeed < 10) {
    tft.setCursor(80, 40);
  } else if (currentSpeed < 100) {
    tft.setCursor(50, 40);
  } else {
    tft.setCursor(20, 40);
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
  tft.setTextSize(3);
  tft.print(" MPH");

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 110);
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
  tft.setCursor(10, 160);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // GPS coordinates (small)
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 190);
    tft.print("LAT: ");
    tft.print(latitude, 5);
    tft.setCursor(10, 202);
    tft.print("LON: ");
    tft.print(longitude, 5);
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 220);
  tft.print("[A]Mode [B]Reset Peak [C]Reset Trip");
}

void drawBillboardScreen() {
  tft.fillScreen(TFT_BLACK);

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("BILLBOARD HOT 100");

  // Title
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 30);
  tft.print("#1 This Week");

  // Decorative line
  tft.drawLine(10, 55, 310, 55, TFT_YELLOW);

  // Song name - big
  tft.setTextSize(3);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 70);

  // Handle long song names
  if (currentSong.length() > 12) {
    tft.print(currentSong.substring(0, 12));
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.print(currentSong.substring(12));
  } else {
    tft.print(currentSong);
  }

  // Artist name
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 130);
  tft.print("by ");
  tft.setTextColor(TFT_WHITE);

  if (currentArtist.length() > 20) {
    tft.print(currentArtist.substring(0, 20));
    tft.setCursor(10, 155);
    tft.print(currentArtist.substring(20));
  } else {
    tft.print(currentArtist);
  }

  // Decorative music notes
  tft.setTextSize(3);
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(260, 70);
  tft.print("~");
  tft.setCursor(280, 90);
  tft.print("~");

  // Chart icon
  tft.drawRect(250, 130, 60, 60, TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(265, 155);
  tft.print("HOT");
  tft.setCursor(265, 165);
  tft.print("100");

  // Button hint
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 220);
  tft.print("[A] Switch to Speedometer");
}
