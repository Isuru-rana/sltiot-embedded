#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <esp_task_wdt.h>

int dhtPin = 15;
int soilPin = 34;
int doorPin = 27;
int buttonPin = 14;
int fanPin = 25;
int pumpPin = 26;
int alarmPin = 4;

DHT dht(dhtPin, DHT22);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

String state = "IDLE";
unsigned long stateStartTime = 0;

float temp = 25;
float hum = 60;
int soil = 50;
int fanSpeed = 0;
bool doorOpen = false;
int lastButton = HIGH;
unsigned long lastSensorTime = 0;
unsigned long lastScreenTime = 0;

void changeState(String newState) {
  Serial.print(state);
  Serial.print(" to ");
  Serial.println(newState);
  state = newState;
  stateStartTime = millis();
}

void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) {
    temp = t;
    hum = h;
  }

  soil = map(analogRead(soilPin), 0, 4095, 0, 100);

  fanSpeed = map(temp, 25, 35, 0, 255);

  if (fanSpeed < 0) {
    fanSpeed = 0;
  }

  if (fanSpeed > 255) {
    fanSpeed = 255;
  }
}

void showScreen() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("GREENHOUSE");

  if (doorOpen == true) {
    oled.print("  DOOR!");
  }

  oled.setTextSize(2);
  oled.setCursor(0, 14);
  oled.println(state);

  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.print(temp, 1);
  oled.print(" C  ");
  oled.print(hum, 0);
  oled.println(" %RH");
  oled.print("Soil ");
  oled.print(soil);
  oled.print(" %  Fan ");
  oled.print(fanSpeed * 100 / 255);
  oled.print(" %");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(doorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);
  ledcAttach(fanPin, 5000, 8);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);

  if (esp_reset_reason() == ESP_RST_TASK_WDT) {
    Serial.println("Boot. Reset reason: WATCHDOG");
  } else {
    Serial.println("Boot. Reset reason: normal");
  }

  esp_task_wdt_config_t config = { .timeout_ms = 5000, .idle_core_mask = 0, .trigger_panic = true };
  esp_task_wdt_reconfigure(&config);
  esp_task_wdt_add(NULL);

  readSensors();
  Serial.println("E6 ready. Turn SOIL below 30 %, press WATER, set temp above 40 C, type h.");
}

void loop() {
  esp_task_wdt_reset();

  if (millis() - lastSensorTime >= 2000) {
    lastSensorTime = millis();
    readSensors();
  }

  if (digitalRead(doorPin) == HIGH) {
    doorOpen = true;
  } else {
    doorOpen = false;
  }

  bool pressed = false;
  int button = digitalRead(buttonPin);

  if (button == LOW && lastButton == HIGH) {
    pressed = true;
  }

  lastButton = button;

  unsigned long timeInState = millis() - stateStartTime;

  if (state == "IDLE") {
    if (temp > 40) {
      changeState("ALARM");
    } else if (soil < 30 || pressed == true) {
      changeState("WATERING");
    }
  } else if (state == "WATERING") {
    if (temp > 40) {
      changeState("ALARM");
    } else if (timeInState >= 5000) {
      changeState("SOAKING");
    }
  } else if (state == "SOAKING") {
    if (temp > 40) {
      changeState("ALARM");
    } else if (timeInState >= 10000) {
      changeState("IDLE");
    }
  } else if (state == "ALARM") {
    if (temp < 38) {
      changeState("IDLE");
    }
  }

  if (state == "WATERING") {
    digitalWrite(pumpPin, HIGH);
  } else {
    digitalWrite(pumpPin, LOW);
  }

  if (state == "ALARM") {
    ledcWrite(fanPin, 255);
  } else if (doorOpen == true) {
    ledcWrite(fanPin, 0);
  } else {
    ledcWrite(fanPin, fanSpeed);
  }

  if (state == "ALARM") {
    if ((millis() / 300) % 2 == 0) {
      digitalWrite(alarmPin, HIGH);
    } else {
      digitalWrite(alarmPin, LOW);
    }
  } else if (doorOpen == true) {
    digitalWrite(alarmPin, HIGH);
  } else {
    digitalWrite(alarmPin, LOW);
  }

  if (millis() - lastScreenTime >= 500) {
    lastScreenTime = millis();
    showScreen();
  }

  if (Serial.available() > 0) {
    char letter = Serial.read();

    if (letter == 'h') {
      Serial.println("Loop frozen. Watchdog resets the board in 5 s...");

      while (true) {
      }
    }
  }
}
