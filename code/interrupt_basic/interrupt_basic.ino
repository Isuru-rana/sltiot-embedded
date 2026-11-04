int blinkPin = 25;
int pollButtonPin = 13;
int pollLedPin = 26;
int intButtonPin = 14;
int intLedPin = 4;

void IRAM_ATTR buttonChanged() {
  if (digitalRead(intButtonPin) == LOW) {
    digitalWrite(intLedPin, HIGH);
  } else {
    digitalWrite(intLedPin, LOW);
  }
}

void setup() {
  pinMode(blinkPin, OUTPUT);
  pinMode(pollLedPin, OUTPUT);
  pinMode(intLedPin, OUTPUT);
  pinMode(pollButtonPin, INPUT_PULLUP);
  pinMode(intButtonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(intButtonPin), buttonChanged, CHANGE);
}

void loop() {
  digitalWrite(blinkPin, HIGH);
  delay(1000);
  digitalWrite(blinkPin, LOW);
  delay(1000);

  if (digitalRead(pollButtonPin) == LOW) {
    digitalWrite(pollLedPin, HIGH);
  } else {
    digitalWrite(pollLedPin, LOW);
  }
}
