#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

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

void setup() {
  Serial.begin(115200);
  pinMode(doorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int soil = map(analogRead(soilPin), 0, 4095, 0, 100);

  int fanSpeed = map(temp, 25, 35, 0, 255);

  if (fanSpeed < 0) {
    fanSpeed = 0;
  }

  if (fanSpeed > 255) {
    fanSpeed = 255;
  }

  int door = digitalRead(doorPin);
  int button = digitalRead(buttonPin);

  if (state == "IDLE") {
    if (temp > 40) {
      state = "ALARM";
    } else if (soil < 30 || button == LOW) {
      state = "WATERING";
    }
  } else if (state == "WATERING") {
    if (temp > 40) {
      state = "ALARM";
    } else if (soil > 60) {
      state = "IDLE";
    }
  } else if (state == "ALARM") {
    if (temp < 38) {
      state = "IDLE";
    }
  }

  if (state == "WATERING") {
    digitalWrite(pumpPin, HIGH);
  } else {
    digitalWrite(pumpPin, LOW);
  }

  if (state == "ALARM") {
    analogWrite(fanPin, 255);
  } else if (door == HIGH) {
    analogWrite(fanPin, 0);
  } else {
    analogWrite(fanPin, fanSpeed);
  }

  if (state == "ALARM" || door == HIGH) {
    digitalWrite(alarmPin, HIGH);
  } else {
    digitalWrite(alarmPin, LOW);
  }

  Serial.print("State: ");
  Serial.print(state);
  Serial.print("  Temp: ");
  Serial.print(temp);
  Serial.print("  Soil: ");
  Serial.println(soil);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("GREENHOUSE");

  if (door == HIGH) {
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

  delay(500);
}
