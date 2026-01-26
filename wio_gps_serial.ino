// GPS Speedometer with Serial Output for localhost server
// Outputs GPS data via USB Serial for Python bridge to read
// NEO-6M GPS on Serial1 (back header pins 8/10)

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
double altitude = 0.0;
bool gpsValid = false;

// Trip data
float tripDistance = 0.0;
double lastLat = 0.0;
double lastLon = 0.0;
bool tripStarted = false;

// Button pins
const int BUTTON_MID = WIO_KEY_B;
const int BUTTON_BOT = WIO_KEY_C;

// Timing
unsigned long lastUpdate = 0;
unsigned long lastSerialSend = 0;
const int UPDATE_INTERVAL = 200;
const int SERIAL_INTERVAL = 500;  // Send to PC every 500ms

void setup() {
  Serial.begin(115200);   // USB Serial to PC
  Serial1.begin(9600);    // GPS on Serial1
  delay(1000);

  Serial.println("=== GPS Speedometer (Serial Mode) ===");

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
  tft.setCursor(30, 70);
  tft.print("GPS SPEEDOMETER");
  tft.setCursor(40, 100);
  tft.print("(Serial Mode)");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 140);
  tft.print("Connect USB & run serial_to_server.py");

  delay(2000);
  tft.fillScreen(TFT_BLACK);

  Serial.println("Waiting for GPS...");
}

void loop() {
  // Read GPS data
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Check buttons
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    peakSpeed = 0.0;
    avgSpeed = 0.0;
    totalSpeed = 0.0;
    speedReadings = 0;
    Serial.println("Peak/Avg reset");
    while (digitalRead(BUTTON_MID) == LOW);
  }

  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    tripDistance = 0.0;
    tripStarted = false;
    Serial.println("Trip reset");
    while (digitalRead(BUTTON_BOT) == LOW);
  }

  // Update display
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateGPSData();
    drawScreen();
  }

  // Send GPS data via Serial to PC
  if (millis() - lastSerialSend >= SERIAL_INTERVAL) {
    lastSerialSend = millis();
    sendSerialData();
  }
}

void sendSerialData() {
  // Format: GPS:lat,lon,speed,altitude,satellites
  Serial.print("GPS:");
  Serial.print(latitude, 6);
  Serial.print(",");
  Serial.print(longitude, 6);
  Serial.print(",");
  Serial.print(currentSpeed, 1);
  Serial.print(",");
  Serial.print(altitude, 1);
  Serial.print(",");
  Serial.println(satellites);
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

void drawScreen() {
  tft.fillScreen(TFT_BLACK);

  // GPS + Serial status
  tft.setTextSize(1);
  tft.setCursor(5, 5);
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

  tft.setCursor(200, 5);
  tft.setTextColor(TFT_MAGENTA);
  tft.print("USB SERIAL");

  // Current speed - BIG
  tft.setTextSize(7);
  int speedX = 100;
  if (currentSpeed >= 100) speedX = 40;
  else if (currentSpeed >= 10) speedX = 70;
  tft.setCursor(speedX, 25);

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
  tft.setCursor(230, 50);
  tft.print("MPH");

  // Altitude
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(180, 80);
  tft.print(altitude * 3.28084, 0);
  tft.print(" ft");

  // Divider
  tft.drawLine(10, 100, 310, 100, TFT_DARKGREY);

  // Peak speed
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 108);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Average speed
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 133);
  tft.print("Avg:  ");
  tft.print(avgSpeed, 1);
  tft.print(" MPH");

  // Trip distance
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(10, 158);
  tft.print("Trip: ");
  tft.print(tripDistance, 2);
  tft.print(" mi");

  // GPS coordinates
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(170, 110);
    tft.print("LAT: ");
    tft.print(latitude, 5);
    tft.setCursor(170, 122);
    tft.print("LON: ");
    tft.print(longitude, 5);
  }

  // Button hints
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 190);
  tft.print("[B] Reset Peak/Avg    [C] Reset Trip");
}
