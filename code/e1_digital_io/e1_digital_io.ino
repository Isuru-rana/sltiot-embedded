// E1 DIGITAL I/O: the door switch and the WATER button
// A digital pin only knows two values: HIGH (3.3 V) or LOW (0 V).

const int DOOR_PIN   = 27;   // slide switch: one side GND (closed), other side 3.3 V (open)
const int BUTTON_PIN = 14;   // push button to GND
const int PUMP_PIN   = 26;   // blue LED
const int ALARM_PIN  = 4;    // red LED

const unsigned long DEBOUNCE_MS = 50;

bool pumpOn = false;
int rawPresses = 0;      // every LOW edge the pin sees, bounces included
int realPresses = 0;     // presses after debounce
int lastRaw = HIGH;
int stableState = HIGH;
unsigned long lastChange = 0;

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_PIN, INPUT_PULLUP);     // inside pull-up: open switch reads HIGH
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  Serial.println("E1 ready. Slide the door switch. Press WATER.");
}

void loop() {
  // 1. Door: read, then act. No memory needed.
  bool doorOpen = digitalRead(DOOR_PIN) == HIGH;
  digitalWrite(ALARM_PIN, doorOpen ? HIGH : LOW);

  // 2. Button: count raw edges, then count debounced presses.
  int raw = digitalRead(BUTTON_PIN);
  if (raw != lastRaw) {
    if (raw == LOW) rawPresses++;
    lastRaw = raw;
    lastChange = millis();
  }
  if (millis() - lastChange > DEBOUNCE_MS && raw != stableState) {
    stableState = raw;
    if (stableState == LOW) {          // a real press
      realPresses++;
      pumpOn = !pumpOn;
      digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);
      Serial.printf("Press! raw edges=%d  real presses=%d  pump=%s  door=%s\n",
                    rawPresses, realPresses, pumpOn ? "ON" : "OFF", doorOpen ? "OPEN" : "CLOSED");
    }
  }
}
