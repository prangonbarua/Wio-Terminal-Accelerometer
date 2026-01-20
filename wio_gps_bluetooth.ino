// Wio Terminal GPS Bluetooth Tracker
// Sends GPS data to phone via Bluetooth Serial
// View data on any Bluetooth Serial app on your phone
// Press TOP button to switch between Speedometer/Stats modes

#include "TFT_eSPI.h"
#include <TinyGPS++.h>
#include <rpcBLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

TFT_eSPI tft;
TinyGPSPlus gps;

// BLE UUIDs
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

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
unsigned long lastBLESend = 0;
const int BLE_SEND_INTERVAL = 1000;  // Send data every 1 second

// BLE connection callback
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Disconnected!");
    }
};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(1000);

  Serial.println("=== Wio GPS Bluetooth Tracker ===");

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
  tft.setCursor(10, 40);
  tft.print("GPS Bluetooth Tracker");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 80);
  tft.print("Starting Bluetooth...");

  // Initialize BLE
  setupBLE();

  tft.setCursor(10, 100);
  tft.setTextColor(TFT_GREEN);
  tft.print("BLE Ready: WioGPS");

  tft.setCursor(10, 120);
  tft.setTextColor(TFT_YELLOW);
  tft.print("Connect with Serial Bluetooth app");

  tft.setCursor(10, 150);
  tft.setTextColor(TFT_WHITE);
  tft.print("Waiting for GPS...");

  delay(3000);
  tft.fillScreen(TFT_BLACK);

  tripStartTime = millis();
}

void setupBLE() {
  BLEDevice::init("WioGPS");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pTxCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE Started - Name: WioGPS");
}

void loop() {
  // Read GPS data
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Handle BLE reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Restarting BLE advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
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

  // Send data via BLE
  if (deviceConnected && gpsValid && (millis() - lastBLESend >= BLE_SEND_INTERVAL)) {
    sendBLEData();
    lastBLESend = millis();
  }
}

void sendBLEData() {
  // Format: LAT,LON,SPEED,ALT,SAT
  String data = String(latitude, 6) + "," +
                String(longitude, 6) + "," +
                String(currentSpeed, 1) + "," +
                String(altitude * 3.28084, 0) + "," +
                String(satellites) + "\n";

  pTxCharacteristic->setValue(data.c_str());
  pTxCharacteristic->notify();

  Serial.print("BLE Sent: ");
  Serial.print(data);
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

  // BLE & GPS status
  tft.setCursor(160, 5);
  if (deviceConnected) {
    tft.setTextColor(TFT_BLUE);
    tft.print("BLE OK ");
  } else {
    tft.setTextColor(TFT_DARKGREY);
    tft.print("BLE -- ");
  }

  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoGPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);

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

  // BLE hint
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(5, 220);
  tft.print("Bluetooth: WioGPS");
}

void drawStatsScreen() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(5, 5);
  tft.print("TRIP STATS");

  // BLE & GPS status
  tft.setCursor(160, 5);
  if (deviceConnected) {
    tft.setTextColor(TFT_BLUE);
    tft.print("BLE OK ");
  } else {
    tft.setTextColor(TFT_DARKGREY);
    tft.print("BLE -- ");
  }

  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS ");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NoGPS ");
  }
  tft.setTextColor(TFT_CYAN);
  tft.print(satellites);

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
