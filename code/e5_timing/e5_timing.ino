int buttonPin = 14;
int pumpPin = 26;
int fanPin = 25;

bool useMillis = false;

bool pumpOn = false;
bool fanOn = false;
int lastButton = HIGH;
unsigned long lastBlinkTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
  pinMode(fanPin, OUTPUT);

  if (useMillis == true) {
    Serial.println("E5 with millis(). Never waits.");
  } else {
    Serial.println("E5 with delay(). Waits 2 seconds each time.");
  }
}

void loop() {
  if (useMillis == true) {
    if (millis() - lastBlinkTime >= 2000) {
      lastBlinkTime = millis();
      fanOn = !fanOn;
      digitalWrite(fanPin, fanOn);
    }
  } else {
    fanOn = !fanOn;
    digitalWrite(fanPin, fanOn);
    delay(2000);
  }

  int button = digitalRead(buttonPin);

  if (button == LOW && lastButton == HIGH) {
    pumpOn = !pumpOn;
    digitalWrite(pumpPin, pumpOn);
    Serial.print(millis());
    Serial.println(" ms: press seen");
    delay(30);
  }

  lastButton = button;
}
