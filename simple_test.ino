// Simple test - just Serial output

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("HELLO!");
  Serial.println("Wio Terminal is working!");
}

void loop() {
  Serial.println("Running...");
  delay(1000);
}
