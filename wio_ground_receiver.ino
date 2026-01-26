// Wio Terminal Ground Station Receiver
// Receives live GPS telemetry from ESP32 on RC Plane
// Displays speed, altitude, position on built-in screen

#include "TFT_eSPI.h"
#include <rpcWiFi.h>
#include <WiFiUdp.h>

TFT_eSPI tft;

// ============================================
// CONFIGURATION - Must match ESP32 transmitter!
// ============================================
const char* AP_SSID = "RC_PLANE_GPS";
const char* AP_PASSWORD = "flight123";
const int UDP_PORT = 4210;
// ============================================

WiFiUDP udp;
char packetBuffer[255];

// Received flight data
double latitude = 0.0;
double longitude = 0.0;
float altitude = 0.0;
float currentSpeed = 0.0;
float peakSpeed = 0.0;
float maxAltitude = 0.0;
int satellites = 0;
bool gpsValid = false;
unsigned long flightTime = 0;

// Connection status
bool wifiConnected = false;
unsigned long lastPacketTime = 0;
int packetsReceived = 0;
bool signalLost = false;

// Button pins
const int BUTTON_TOP = WIO_KEY_A;
const int BUTTON_MID = WIO_KEY_B;
const int BUTTON_BOT = WIO_KEY_C;

// Timing
unsigned long lastDisplayUpdate = 0;
const int DISPLAY_INTERVAL = 100;  // 10Hz display update

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("Wio Terminal Ground Station");
  Serial.println("================================");

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
  tft.print("GROUND STATION");
  tft.setCursor(30, 80);
  tft.print("RC PLANE TRACKER");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 120);
  tft.print("Connecting to: ");
  tft.print(AP_SSID);

  // Connect to ESP32 Access Point
  connectToPlane();

  delay(1000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // Check for incoming UDP packets
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
      parsePacket(packetBuffer);
      lastPacketTime = millis();
      packetsReceived++;
      signalLost = false;
    }
  }

  // Check for signal loss (no packet for 2 seconds)
  if (millis() - lastPacketTime > 2000 && lastPacketTime > 0) {
    signalLost = true;
  }

  // Check buttons
  checkButtons();

  // Update display
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    drawTelemetryScreen();
    lastDisplayUpdate = millis();
  }

  // Try to reconnect if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    connectToPlane();
  }
}

void connectToPlane() {
  Serial.print("Connecting to ");
  Serial.println(AP_SSID);

  WiFi.begin(AP_SSID, AP_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    tft.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Start listening for UDP
    udp.begin(UDP_PORT);
    Serial.print("Listening on UDP port: ");
    Serial.println(UDP_PORT);

    tft.setTextColor(TFT_GREEN);
    tft.setCursor(30, 140);
    tft.print("Connected! IP: ");
    tft.print(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("\nConnection failed!");

    tft.setTextColor(TFT_RED);
    tft.setCursor(30, 140);
    tft.print("Connection FAILED!");
  }
}

void parsePacket(char* data) {
  // Format: LAT,LON,ALT,SPEED,PEAK,MAXALT,SATS,VALID,TIME
  char* token = strtok(data, ",");
  int field = 0;

  while (token != NULL) {
    switch (field) {
      case 0: latitude = atof(token); break;
      case 1: longitude = atof(token); break;
      case 2: altitude = atof(token); break;
      case 3: currentSpeed = atof(token); break;
      case 4: peakSpeed = atof(token); break;
      case 5: maxAltitude = atof(token); break;
      case 6: satellites = atoi(token); break;
      case 7: gpsValid = (atoi(token) == 1); break;
      case 8: flightTime = atol(token); break;
    }
    token = strtok(NULL, ",");
    field++;
  }
}

void checkButtons() {
  // MIDDLE button - reset peak speed display
  if (digitalRead(BUTTON_MID) == LOW) {
    delay(200);
    // Local reset only (plane keeps tracking)
    while (digitalRead(BUTTON_MID) == LOW);
  }

  // BOTTOM button - reconnect
  if (digitalRead(BUTTON_BOT) == LOW) {
    delay(200);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(30, 100);
    tft.setTextColor(TFT_YELLOW);
    tft.print("Reconnecting...");
    WiFi.disconnect();
    delay(500);
    connectToPlane();
    while (digitalRead(BUTTON_BOT) == LOW);
  }
}

void drawTelemetryScreen() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.setTextColor(TFT_DARKGREY);
  tft.print("RC PLANE TELEMETRY");

  // Connection status
  tft.setCursor(200, 5);
  if (!wifiConnected) {
    tft.setTextColor(TFT_RED);
    tft.print("NO LINK");
  } else if (signalLost) {
    tft.setTextColor(TFT_ORANGE);
    tft.print("SIGNAL LOST");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.print("LINKED ");
    tft.setTextColor(TFT_CYAN);
    tft.print(satellites);
    tft.print(" SAT");
  }

  // GPS status indicator
  tft.setCursor(5, 15);
  if (gpsValid) {
    tft.setTextColor(TFT_GREEN);
    tft.print("GPS LOCKED");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("NO GPS FIX");
  }

  // Packets received
  tft.setCursor(200, 15);
  tft.setTextColor(TFT_MAGENTA);
  tft.print("PKT: ");
  tft.print(packetsReceived);

  // ===== SPEED (BIG) =====
  tft.setTextSize(6);
  int speedX = 70;
  if (currentSpeed >= 100) speedX = 40;
  else if (currentSpeed >= 10) speedX = 55;
  tft.setCursor(speedX, 35);

  // Color based on speed
  if (currentSpeed < 20) {
    tft.setTextColor(TFT_GREEN);
  } else if (currentSpeed < 50) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  tft.print(currentSpeed, 0);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(220, 55);
  tft.print("MPH");

  // ===== ALTITUDE =====
  tft.setTextSize(3);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(180, 85);
  float altFeet = altitude * 3.28084;
  tft.print(altFeet, 0);
  tft.setTextSize(2);
  tft.print(" ft");

  // Divider
  tft.drawLine(10, 115, 310, 115, TFT_DARKGREY);

  // ===== STATS =====
  tft.setTextSize(2);

  // Peak speed
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 125);
  tft.print("Peak: ");
  tft.setTextColor(TFT_RED);
  tft.print(peakSpeed, 1);
  tft.print(" MPH");

  // Max altitude
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 148);
  tft.print("Max Alt: ");
  tft.setTextColor(TFT_ORANGE);
  tft.print(maxAltitude * 3.28084, 0);
  tft.print(" ft");

  // Flight time
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 171);
  tft.print("Flight: ");
  int mins = flightTime / 60;
  int secs = flightTime % 60;
  tft.print(mins);
  tft.print("m ");
  tft.print(secs);
  tft.print("s");

  // ===== COORDINATES =====
  if (gpsValid) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(170, 130);
    tft.print("LAT: ");
    tft.print(latitude, 5);
    tft.setCursor(170, 142);
    tft.print("LON: ");
    tft.print(longitude, 5);
  }

  // ===== SIGNAL STRENGTH BAR =====
  int signalAge = millis() - lastPacketTime;
  int barWidth = 0;
  uint16_t barColor = TFT_GREEN;

  if (signalAge < 200) {
    barWidth = 100;
    barColor = TFT_GREEN;
  } else if (signalAge < 500) {
    barWidth = 75;
    barColor = TFT_YELLOW;
  } else if (signalAge < 1000) {
    barWidth = 50;
    barColor = TFT_ORANGE;
  } else if (signalAge < 2000) {
    barWidth = 25;
    barColor = TFT_RED;
  } else {
    barWidth = 5;
    barColor = TFT_RED;
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(200, 170);
  tft.print("SIGNAL");
  tft.fillRect(200, 180, barWidth, 8, barColor);
  tft.drawRect(200, 180, 100, 8, TFT_DARKGREY);

  // ===== BUTTON HINTS =====
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 200);
  tft.print("[C] Reconnect");

  // Warning if signal lost
  if (signalLost) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(80, 210);
    tft.print("! SIGNAL LOST !");
  }
}
