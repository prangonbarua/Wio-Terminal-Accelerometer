// Wio Terminal GPS Multi-Mode Display
// Mode 1: Speedometer - Big speed display
// Mode 2: Map - Draw your path on screen
// Mode 3: Stats - Trip summary
// Press TOP button (A) to switch modes

#include "TFT_eSPI.h"
#include <TinyGPS++.h>

TFT_eSPI tft;
TinyGPSPlus gps;

// Modes
enum Mode { SPEEDOMETER, MAP, STATS };
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
float tripDistance = 0.0; // miles
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;
unsigned long tripStartTime = 0;

// Map tracking - store path points
const int MAX_PATH_POINTS = 500;
int16_t pathX[MAX_PATH_POINTS];
int16_t pathY[MAX_PATH_POINTS];
int pathCount = 0;

// Map bounds (auto-scaling)
double minLat = 999.0, maxLat = -999.0;
double minLon = 999.0, maxLon = -999.0;

// Screen dimensions for map
const int MAP_X = 10;
const int MAP_Y = 25;
const int MAP_WIDTH = 300;
const int MAP_HEIGHT = 180;

// Button pins
const int BUTTON_TOP = WIO_KEY_A;    // Top button - switch mode
const int BUTTON_MID = WIO_KEY_B;    // Middle button - reset peak/zoom
const int BUTTON_BOT = WIO_KEY_C;    // Bottom button - reset trip/clear map

// Timing
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200; // 5Hz update rate
unsigned long lastPathSave = 0;
const int PATH_SAVE_INTERVAL = 2000; // Save path point every 2 seconds

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); // GPS on Serial1
  delay(1000);

  Serial.println("=== Wio Terminal GPS Multi-Mode ===");

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
  tft.print("GPS MULTI-MODE");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 90);
  tft.print("1. SPEEDOMETER - Speed display");
  tft.setCursor(30, 110);
  tft.print("2. MAP - Draw your path");
  tft.setCursor(30, 130);
  tft.print("3. STATS - Trip summary");

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 160);
  tft.print("Press TOP button to switch modes");

  tft.setTextColor(TFT_GREEN);
  tft.setCursor(30, 190);
  tft.print("Waiting for GPS signal...");

  delay(3000);
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

  // Update display at set interval
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    // Update GPS data
    updateGPSData();

    // Save path point for map
    if (gpsValid && (millis() - lastPathSave >= PATH_SAVE_INTERVAL)) {
      savePathPoint();
      lastPathSave = millis();
    }

    // Draw current mode
    switch (currentMode) {
      case SPEEDOMETER:
        drawSpeedometerScreen();
        break;
      case MAP:
        drawMapScreen();
        break;
      case STATS:
        drawStatsScreen();
        break;
    }
  }
}

void checkButtons() {
  // TOP button - switch mode
  if (digitalRead(BUTTON_TOP) == LOW) {
    delay(200);
    currentMode = (Mode)((currentMode + 1) % 3);
    Serial.print("Mode: ");
    Serial.println(currentMode == SPEEDOMETER ? "SPEEDOMETER" : (currentMode == MAP ? "MAP" : "STATS"));
    tft.fillScreen(TFT_BLACK);
    while (digitalRead(BUTTON_TOP) == LOW);
  }

  // MIDDLE button - reset peak (speedometer) or zoom fit (map)
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    if (currentMode == SPEEDOMETER) {
      peakSpeed = 0.0;
      avgSpeed = 0.0;
      totalSpeed = 0.0;
      speedReadings = 0;
      Serial.println("Peak speed reset");
    } else if (currentMode == MAP) {
      // Recenter map
      Serial.println("Map recentered");
    }
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // BOTTOM button - reset trip / clear map
  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    tripDistance = 0.0;
    tripStarted = false;
    tripStartTime = millis();
    pathCount = 0;
    minLat = 999.0; maxLat = -999.0;
    minLon = 999.0; maxLon = -999.0;
    peakSpeed = 0.0;
    avgSpeed = 0.0;
    totalSpeed = 0.0;
    speedReadings = 0;
    Serial.println("Trip and map reset");
    tft.fillScreen(TFT_BLACK);
    while (digitalRead(BUTTON_BOT) == LOW);
  }
}

void updateGPSData() {
  if (gps.location.isValid()) {
    gpsValid = true;
    latitude = gps.location.lat();
    longitude = gps.location.lng();

    // Update map bounds
    if (latitude < minLat) minLat = latitude;
    if (latitude > maxLat) maxLat = latitude;
    if (longitude < minLon) minLon = longitude;
    if (longitude > maxLon) maxLon = longitude;

    if (gps.altitude.isValid()) {
      altitude = gps.altitude.meters();
    }

    // Get speed in MPH
    if (gps.speed.isValid()) {
      currentSpeed = gps.speed.mph();

      // Update peak
      if (currentSpeed > peakSpeed) {
        peakSpeed = currentSpeed;
      }

      // Update average (only when moving)
      if (currentSpeed > 0.5) {
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

void savePathPoint() {
  if (!gpsValid || pathCount >= MAX_PATH_POINTS) return;

  // Convert lat/lon to screen coordinates
  int16_t x, y;
  latLonToScreen(latitude, longitude, x, y);

  pathX[pathCount] = x;
  pathY[pathCount] = y;
  pathCount++;

  Serial.print("Path point saved: ");
  Serial.print(pathCount);
  Serial.print(" (");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");
}

void latLonToScreen(double lat, double lon, int16_t &x, int16_t &y) {
  // Add padding to bounds
  double latRange = maxLat - minLat;
  double lonRange = maxLon - minLon;

  // Minimum range to prevent division by zero
  if (latRange < 0.0001) latRange = 0.0001;
  if (lonRange < 0.0001) lonRange = 0.0001;

  // Add 10% padding
  double padLat = latRange * 0.1;
  double padLon = lonRange * 0.1;

  double adjMinLat = minLat - padLat;
  double adjMaxLat = maxLat + padLat;
  double adjMinLon = minLon - padLon;
  double adjMaxLon = maxLon + padLon;

  // Map to screen coordinates
  x = MAP_X + (int16_t)(((lon - adjMinLon) / (adjMaxLon - adjMinLon)) * MAP_WIDTH);
  y = MAP_Y + MAP_HEIGHT - (int16_t)(((lat - adjMinLat) / (adjMaxLat - adjMinLat)) * MAP_HEIGHT);

  // Clamp to map area
  if (x < MAP_X) x = MAP_X;
  if (x > MAP_X + MAP_WIDTH) x = MAP_X + MAP_WIDTH;
  if (y < MAP_Y) y = MAP_Y;
  if (y > MAP_Y + MAP_HEIGHT) y = MAP_Y + MAP_HEIGHT;
}

void drawSpeedometerScreen() {
  tft.fillScreen(TFT_BLACK);

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("MODE: SPEEDOMETER");

  // GPS status
  tft.setCursor(200, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS ");
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
  tft.setCursor(200, 95);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Divider line
  tft.drawLine(10, 115, 310, 115, TFT_DARKGREY);

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 125);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average speed
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
  tft.setCursor(5, 200);
  tft.print("[A] Map Mode  [B] Reset Peak  [C] Reset All");
}

void drawMapScreen() {
  tft.fillScreen(TFT_BLACK);

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("MODE: MAP");

  // GPS status
  tft.setCursor(200, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

  // Path points counter
  tft.setCursor(100, 5);
  tft.setTextColor(TFT_YELLOW);
  tft.print(pathCount);
  tft.print(" pts");

  // Draw map border
  tft.drawRect(MAP_X - 1, MAP_Y - 1, MAP_WIDTH + 2, MAP_HEIGHT + 2, TFT_DARKGREY);

  // Draw path
  if (pathCount > 1) {
    // Recalculate all points with current bounds
    for (int i = 1; i < pathCount; i++) {
      // Get original lat/lon from stored points (we need to recalc)
      // For now, draw stored screen points
      // Draw line segment
      tft.drawLine(pathX[i-1], pathY[i-1], pathX[i], pathY[i], TFT_GREEN);
    }
  }

  // Draw current position
  if (gpsValid) {
    int16_t curX, curY;
    latLonToScreen(latitude, longitude, curX, curY);

    // Draw crosshair at current position
    tft.fillCircle(curX, curY, 4, TFT_RED);
    tft.drawCircle(curX, curY, 6, TFT_WHITE);
  }

  // Draw start point
  if (pathCount > 0) {
    tft.fillCircle(pathX[0], pathY[0], 3, TFT_BLUE);
  }

  // Info bar at bottom
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 210);
  tft.print("Dist: ");
  tft.setTextColor(TFT_GREEN);
  tft.print(tripDistance, 2);
  tft.print(" mi");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(120, 210);
  tft.print("Speed: ");
  tft.setTextColor(TFT_YELLOW);
  tft.print(currentSpeed, 0);
  tft.print(" MPH");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(220, 210);
  tft.print("Alt: ");
  tft.setTextColor(TFT_ORANGE);
  tft.print(altitude * 3.28084, 0);
  tft.print("ft");

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 225);
  tft.print("[A] Stats Mode  [C] Clear Map");
}

void drawStatsScreen() {
  tft.fillScreen(TFT_BLACK);

  // Mode indicator
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("MODE: STATS");

  // GPS status
  tft.setCursor(200, 5);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);
  tft.print(" SAT");

  // Title
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(80, 25);
  tft.print("TRIP SUMMARY");

  // Divider
  tft.drawLine(10, 50, 310, 50, TFT_DARKGREY);

  // Trip duration
  unsigned long tripDuration = millis() - tripStartTime;
  int hours = tripDuration / 3600000;
  int minutes = (tripDuration % 3600000) / 60000;
  int seconds = (tripDuration % 60000) / 1000;

  tft.setTextSize(2);
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

  // Total distance
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 85);
  tft.print("Distance: ");
  tft.setTextColor(TFT_MAGENTA);
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // Current speed
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 110);
  tft.print("Current:  ");
  tft.setTextColor(TFT_GREEN);
  tft.print(currentSpeed, 1);
  tft.print(" MPH");

  // Peak speed
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 135);
  tft.print("Peak:     ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average speed
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 160);
  tft.print("Average:  ");
  tft.setTextColor(TFT_CYAN);
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Current altitude
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 185);
  tft.print("Altitude: ");
  tft.setTextColor(TFT_ORANGE);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

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
  tft.print("[A] Speed Mode  [B] Reset Stats  [C] Reset All");
}
