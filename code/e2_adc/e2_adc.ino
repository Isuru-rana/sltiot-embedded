int soilPin = 34;

void setup() {
  Serial.begin(115200);
  Serial.println("E2 ready. Turn the SOIL knob.");
}

void loop() {
  long total = 0;

  for (int i = 0; i < 16; i++) {
    total = total + analogRead(soilPin);
  }

  int raw = total / 16;
  float volts = raw * 3.3 / 4095;
  int moisture = map(raw, 0, 4095, 0, 100);

  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print("   Volts: ");
  Serial.print(volts);
  Serial.print("   Moisture: ");
  Serial.print(moisture);
  Serial.print(" %");

  if (moisture < 30) {
    Serial.println("   DRY, needs water");
  } else {
    Serial.println("   OK");
  }

  delay(1000);
}
