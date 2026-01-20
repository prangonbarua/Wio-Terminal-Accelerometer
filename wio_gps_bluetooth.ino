// Wio Terminal GPS Tracker with USB Serial Output
// Connect Wio Terminal to phone/computer via USB
// Use Serial Monitor or any serial app to view GPS data
// Press TOP button to switch between Speedometer/Stats modes

#include "TFT_eSPI.h"
#include <TinyGPS++.h>

TFT_eSPI tft;
TinyGPSPlus gps;

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

// Button pins
const int BUTTON_TOP = WIO_KEY_A;
const int BUTTON_MID = WIO_KEY_B;
const int BUTTON_BOT = WIO_KEY_C;

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200;
unsigned long lastSerialSend = 0;
const int SERIAL_SEND_INTERVAL = 1000;  // Send data every 1 second

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(1000);

  Serial.println("=== Wio GPS Tracker ===");
  Serial.println("Format: LAT,LON,SPEED_MPH,ALT_FT,SATELLITES");

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
  tft.setCursor(30, 50);
  tft.print("GPS Tracker");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 90);
  tft.print("GPS data streams via USB Serial");
  tft.setCursor(30, 110);
  tft.print("Open Serial Monitor to view");

  tft.setTextColor(TFT_GREEN);
  tft.setCursor(30, 140);
  tft.print("Waiting for GPS...");

  delay(2500);
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

  // Send data via Serial
  if (gpsValid && (millis() - lastSerialSend >= SERIAL_SEND_INTERVAL)) {
    sendSerialData();
    lastSerialSend = millis();
  }
}

void sendSerialData() {
  // Format: LAT,LON,SPEED,ALT,SAT
  Serial.print(latitude, 6);
  Serial.print(",");
  Serial.print(longitude, 6);
  Serial.print(",");
  Serial.print(currentSpeed, 1);
  Serial.print(",");
  Serial.print(altitude * 3.28084, 0);
  Serial.print(",");
  Serial.println(satellites);
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

  // GPS status
  tft.setCursor(200, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoGPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

  // Current speed - BIG
  tft.setTextSize(7);
  if (currentSpeed < 10) {
    tft.setCursor(100, 35);
  } else if (currentSpeed < 100) {
    tft.setCursor(70, 35);
  } else {
    tft.setCursor(40, 35);
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
  tft.setCursor(230, 60);
  tft.print("MPH");

  // Altitude
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(200, 95);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Divider
  tft.drawLine(10, 115, 310, 115, TFT_DARKGREY);

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 125);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 150);
  tft.print("Avg:  ");
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Trip distance
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 175);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 205);
  tft.print("[A] Stats  [B] Reset Peak  [C] Reset All");

  // Serial hint
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 220);
  tft.print("Data streaming via USB Serial");
}

void drawStatsScreen() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("TRIP STATS");

  // GPS status
  tft.setCursor(200, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoGPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

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

  // Coordinates
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 190);
    tft.print("LAT: ");
    tft.print(latitude, 6);
    tft.setCursor(10, 205);
    tft.print("LON: ");
    tft.print(longitude, 6);
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 225);
  tft.print("[A] Speed  [B] Reset Peak  [C] Reset All");
}
