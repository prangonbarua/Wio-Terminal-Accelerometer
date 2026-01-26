// ESP32 RC Plane GPS Transmitter
// Mounts on RC plane with NEO-6M GPS
// Creates WiFi hotspot and broadcasts GPS data via UDP
// Wio Terminal connects and receives live telemetry

#include <WiFi.h>
#include <WiFiUdp.h>
#include <TinyGPS++.h>

// ============================================
// CONFIGURATION
// ============================================
const char* AP_SSID = "RC_PLANE_GPS";
const char* AP_PASSWORD = "flight123";
const int UDP_PORT = 4210;
const int BROADCAST_INTERVAL = 100;  // 10Hz update rate
// ============================================

// GPS Serial pins for ESP32-C3
// Try GPIO 4 (RX) and GPIO 5 (TX) - these are safer pins
#define GPS_RX 4   // Connect GPS TX to this pin
#define GPS_TX 5   // Connect GPS RX to this pin

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // Use Serial1
WiFiUDP udp;

// Flight data
float currentSpeed = 0.0;
float peakSpeed = 0.0;
float maxAltitude = 0.0;
double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;
int satellites = 0;
bool gpsValid = false;
unsigned long flightStartTime = 0;

// Timing
unsigned long lastBroadcast = 0;
unsigned long lastGPSUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 RC Plane GPS Transmitter");
  Serial.println("================================");

  // Initialize GPS serial
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS initialized on Serial2");
  Serial.print("  RX Pin: "); Serial.println(GPS_RX);
  Serial.print("  TX Pin: "); Serial.println(GPS_TX);

  // Create WiFi Access Point
  Serial.println("\nCreating WiFi Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP SSID: "); Serial.println(AP_SSID);
  Serial.print("AP Password: "); Serial.println(AP_PASSWORD);
  Serial.print("AP IP address: "); Serial.println(IP);

  // Start UDP
  udp.begin(UDP_PORT);
  Serial.print("UDP broadcasting on port: "); Serial.println(UDP_PORT);

  flightStartTime = millis();

  Serial.println("\nWaiting for GPS signal...");
  Serial.println("================================");
}

void loop() {
  // Read GPS data
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  // Update GPS data
  updateGPSData();

  // Broadcast data at set interval
  if (millis() - lastBroadcast >= BROADCAST_INTERVAL) {
    broadcastData();
    lastBroadcast = millis();
  }

  // Print status every 2 seconds
  if (millis() - lastGPSUpdate >= 2000) {
    printStatus();
    lastGPSUpdate = millis();
  }
}

void updateGPSData() {
  if (gps.location.isValid()) {
    gpsValid = true;
    latitude = gps.location.lat();
    longitude = gps.location.lng();

    if (gps.altitude.isValid()) {
      altitude = gps.altitude.meters();
      if (altitude > maxAltitude) {
        maxAltitude = altitude;
      }
    }

    if (gps.speed.isValid()) {
      currentSpeed = gps.speed.mph();
      if (currentSpeed > peakSpeed) {
        peakSpeed = currentSpeed;
      }
    }
  } else {
    gpsValid = false;
  }

  satellites = gps.satellites.value();
}

void broadcastData() {
  // Create data packet
  // Format: LAT,LON,ALT,SPEED,PEAK,MAXALT,SATS,VALID,TIME
  String packet = "";
  packet += String(latitude, 6) + ",";
  packet += String(longitude, 6) + ",";
  packet += String(altitude, 1) + ",";
  packet += String(currentSpeed, 1) + ",";
  packet += String(peakSpeed, 1) + ",";
  packet += String(maxAltitude, 1) + ",";
  packet += String(satellites) + ",";
  packet += String(gpsValid ? 1 : 0) + ",";
  packet += String((millis() - flightStartTime) / 1000);

  // Broadcast to all connected clients
  IPAddress broadcastIP(192, 168, 4, 255);
  udp.beginPacket(broadcastIP, UDP_PORT);
  udp.print(packet);
  udp.endPacket();
}

void printStatus() {
  Serial.println("--- Status ---");
  Serial.print("GPS: ");
  if (gpsValid) {
    Serial.println("LOCKED");
    Serial.print("  Lat: "); Serial.println(latitude, 6);
    Serial.print("  Lon: "); Serial.println(longitude, 6);
    Serial.print("  Alt: "); Serial.print(altitude); Serial.println(" m");
    Serial.print("  Speed: "); Serial.print(currentSpeed); Serial.println(" mph");
  } else {
    Serial.println("SEARCHING...");
  }
  Serial.print("  Satellites: "); Serial.println(satellites);
  Serial.print("  Peak Speed: "); Serial.print(peakSpeed); Serial.println(" mph");
  Serial.print("  Max Alt: "); Serial.print(maxAltitude); Serial.println(" m");
  Serial.print("  Clients: "); Serial.println(WiFi.softAPgetStationNum());
  Serial.println();
}
