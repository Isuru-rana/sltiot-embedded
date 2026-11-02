// E6 STATE MACHINE: the complete greenhouse controller (Day 1 final)
// States: IDLE -> WATERING -> SOAKING -> IDLE, and ALARM from anywhere.
// Fan speed follows temperature (PWM) in every state except ALARM.
// Watchdog: type h in the serial monitor to freeze the loop. The ESP32 reboots itself.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <esp_task_wdt.h>

const int DHT_PIN = 15, SOIL_PIN = 34, DOOR_PIN = 27, BUTTON_PIN = 14;
const int FAN_PIN = 25, PUMP_PIN = 26, ALARM_PIN = 4;

const int DRY_BELOW = 30;              // % soil moisture
const float HOT_C = 40.0;              // alarm
const float FAN_START_C = 25.0, FAN_FULL_C = 35.0;
const unsigned long WATER_MS = 5000;   // pump run time
const unsigned long SOAK_MS = 10000;   // let the water spread before checking again

enum State { IDLE, WATERING, SOAKING, ALARM };
const char* NAMES[] = { "IDLE", "WATERING", "SOAKING", "ALARM" };
State state = IDLE;
unsigned long stateStart = 0;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DHT dht(DHT_PIN, DHT22);
float temp = 25, hum = 60;
int soil = 50;
bool doorOpen = false, buttonWas = false;
unsigned long lastSensor = 0, lastScreen = 0;

void enter(State s) {
  Serial.printf("%s -> %s\n", NAMES[state], NAMES[s]);
  state = s;
  stateStart = millis();
}

unsigned long inState() { return millis() - stateStart; }

int fanDuty() {
  if (temp <= FAN_START_C) return 0;
  if (temp >= FAN_FULL_C) return 255;
  return (temp - FAN_START_C) * 255 / (FAN_FULL_C - FAN_START_C);
}

void readSensors() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t)) { temp = t; hum = h; }
  soil = map(analogRead(SOIL_PIN), 0, 4095, 0, 100);
}

void showScreen() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("GREENHOUSE  %s\n", doorOpen ? "DOOR!" : "");
  oled.setTextSize(2);
  oled.setCursor(0, 14);
  oled.println(NAMES[state]);
  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.printf("%.1f C  %.0f %%RH\nSoil %d %%  Fan %d %%", temp, hum, soil, fanDuty() * 100 / 255);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  ledcAttach(FAN_PIN, 5000, 8);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);

  Serial.printf("Boot. Reset reason: %d (%s)\n", esp_reset_reason(),
                esp_reset_reason() == ESP_RST_TASK_WDT ? "WATCHDOG" : "normal");

  esp_task_wdt_config_t cfg = { .timeout_ms = 5000, .idle_core_mask = 0, .trigger_panic = true };
  if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) esp_task_wdt_init(&cfg);
  esp_task_wdt_add(NULL);              // watch this loop
  readSensors();
  Serial.println("E6 ready. Turn SOIL below 30 %, press WATER, set temp above 40 C, type h.");
}

void loop() {
  esp_task_wdt_reset();                // "I am still alive"

  if (millis() - lastSensor >= 2000) { lastSensor = millis(); readSensors(); }
  doorOpen = digitalRead(DOOR_PIN) == HIGH;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  bool newPress = pressed && !buttonWas;
  buttonWas = pressed;

  // ---- the state machine: one switch, every rule in one place ----
  switch (state) {
    case IDLE:
      if (temp > HOT_C) enter(ALARM);
      else if (soil < DRY_BELOW || newPress) enter(WATERING);
      break;
    case WATERING:
      if (temp > HOT_C) enter(ALARM);
      else if (inState() >= WATER_MS) enter(SOAKING);
      break;
    case SOAKING:
      if (temp > HOT_C) enter(ALARM);
      else if (inState() >= SOAK_MS) enter(IDLE);
      break;
    case ALARM:
      if (temp < HOT_C - 2) enter(IDLE);   // 2 C gap stops flicker
      break;
  }

  // ---- outputs follow the state ----
  digitalWrite(PUMP_PIN, state == WATERING);
  ledcWrite(FAN_PIN, state == ALARM ? 255 : (doorOpen ? 0 : fanDuty()));
  bool blink = (millis() / 300) % 2;
  digitalWrite(ALARM_PIN, state == ALARM ? blink : doorOpen);

  if (millis() - lastScreen >= 500) { lastScreen = millis(); showScreen(); }

  if (Serial.available() && Serial.read() == 'h') {
    Serial.println("Simulating a bug: loop frozen. Watchdog fires in 5 s...");
    while (true) { }                   // stuck forever, no esp_task_wdt_reset()
  }
}
