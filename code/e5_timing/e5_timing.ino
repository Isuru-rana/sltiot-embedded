// E5 TIMING: why delay() makes a device deaf
// The FAN LED blinks every 2 s. Press WATER and see how fast the pump reacts.
// USE_MILLIS 0: blink with delay(). Presses are missed.
// USE_MILLIS 1: blink with millis(). Every press works at once.

#define USE_MILLIS 0

const int BUTTON_PIN = 14;
const int PUMP_PIN   = 26;
const int FAN_PIN    = 25;

bool pumpOn = false;
bool fanOn = false;
int lastButton = HIGH;
unsigned long lastBlink = 0;

void checkButton() {
  int b = digitalRead(BUTTON_PIN);
  if (b == LOW && lastButton == HIGH) {
    pumpOn = !pumpOn;
    digitalWrite(PUMP_PIN, pumpOn);
    Serial.printf("%lu ms: press seen, pump %s\n", millis(), pumpOn ? "ON" : "OFF");
    delay(30);   // crude debounce
  }
  lastButton = b;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  Serial.println(USE_MILLIS ? "E5 with millis(): never blocks" : "E5 with delay(): blocks 2 s at a time");
}

void loop() {
#if USE_MILLIS
  if (millis() - lastBlink >= 2000) {    // is it time yet? no waiting
    lastBlink = millis();
    fanOn = !fanOn;
    digitalWrite(FAN_PIN, fanOn);
  }
#else
  fanOn = !fanOn;
  digitalWrite(FAN_PIN, fanOn);
  delay(2000);                           // the CPU does nothing else here
#endif
  checkButton();
}
