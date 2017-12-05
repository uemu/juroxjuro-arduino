void setup() {
  pinMode(2, OUTPUT); // 2番はビルトインのLED
}

void loop() {
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  delay(1000);
}
