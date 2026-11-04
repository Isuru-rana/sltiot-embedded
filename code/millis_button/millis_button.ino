int buttonPin = 14;
int lastButton = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Press the button");
}

void loop() {
  int button = digitalRead(buttonPin);

  if (button == LOW && lastButton == HIGH) {
    unsigned long ms = millis();

    Serial.print(ms / 1000.0, 3);
    Serial.print(" s   ");
    Serial.print(ms);
    Serial.println(" ms");

    delay(50);
  }

  lastButton = button;
}
