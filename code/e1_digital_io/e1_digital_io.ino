int doorPin = 27;
int buttonPin = 14;
int pumpPin = 26;
int alarmPin = 4;

bool pumpOn = false;
int lastButton = HIGH;
unsigned long lastChangeTime = 0;
int rawPresses = 0;
int realPresses = 0;

void setup() {
  Serial.begin(115200);
  pinMode(doorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);
  Serial.println("E1 ready. Slide the door switch. Press WATER.");
}

void loop() {
  int door = digitalRead(doorPin);

  if (door == HIGH) {
    digitalWrite(alarmPin, HIGH);
  } else {
    digitalWrite(alarmPin, LOW);
  }

  int button = digitalRead(buttonPin);

  if (button != lastButton) {
    if (button == LOW) {
      rawPresses++;

      if (millis() - lastChangeTime > 50) {
        realPresses++;
        pumpOn = !pumpOn;

        if (pumpOn == true) {
          digitalWrite(pumpPin, HIGH);
        } else {
          digitalWrite(pumpPin, LOW);
        }

        Serial.print("Raw presses: ");
        Serial.print(rawPresses);
        Serial.print("   Real presses: ");
        Serial.println(realPresses);
      }
    }

    lastChangeTime = millis();
    lastButton = button;
  }
}
