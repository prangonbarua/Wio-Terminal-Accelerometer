// Backlight test for Wio Terminal
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Testing backlight...");

  // Try turning on backlight with pin 72
  pinMode(72, OUTPUT);

  while(true) {
    Serial.println("Backlight HIGH");
    digitalWrite(72, HIGH);
    delay(2000);

    Serial.println("Backlight LOW");
    digitalWrite(72, LOW);
    delay(2000);
  }
}

void loop() {
  // Nothing
}
