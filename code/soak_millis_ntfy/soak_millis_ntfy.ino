#include <WiFi.h>
#include <HTTPClient.h>
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
String lastState = "";
String ntfyTopic = "sltiot-demo-change-me";
unsigned long startTime = 0;

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

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int soil = analogRead(soilPin) * 100 / 4095;

  int door = digitalRead(doorPin);
  int button = digitalRead(buttonPin);

  if (state == "IDLE") {
    digitalWrite(pumpPin, LOW);
    digitalWrite(fanPin, LOW);

    if (door == HIGH) {
      digitalWrite(alarmPin, HIGH);
    } else {
      digitalWrite(alarmPin, LOW);
    }

    if (temp > 40) {
      state = "ALARM";
    } else if (soil < 30 || button == LOW) {
      state = "WATERING";
      startTime = millis();
    }
  } else if (state == "WATERING") {
    digitalWrite(pumpPin, HIGH);
    digitalWrite(fanPin, LOW);

    if (door == HIGH) {
      digitalWrite(alarmPin, HIGH);
    } else {
      digitalWrite(alarmPin, LOW);
    }

    if (temp > 40) {
      state = "ALARM";
    } else if (millis() - startTime >= 5000) {
      state = "SOAKING";
      startTime = millis();
    }
  } else if (state == "SOAKING") {
    digitalWrite(pumpPin, LOW);
    digitalWrite(fanPin, LOW);

    if (door == HIGH) {
      digitalWrite(alarmPin, HIGH);
    } else {
      digitalWrite(alarmPin, LOW);
    }

    if (temp > 40) {
      state = "ALARM";
    } else if (millis() - startTime >= 10000) {
      state = "IDLE";
    }
  } else if (state == "ALARM") {
    digitalWrite(pumpPin, LOW);
    digitalWrite(fanPin, HIGH);
    digitalWrite(alarmPin, HIGH);

    if (temp < 38) {
      state = "IDLE";
    }
  }

  if (state != lastState) {
    lastState = state;
    String message = "State: " + state + "  Temp: " + String(temp, 1) + " C  Soil: " + String(soil) + " %";

    HTTPClient http;
    http.begin("http://ntfy.sh/" + ntfyTopic);
    http.POST(message);
    http.end();

    Serial.print("Sent: ");
    Serial.println(message);
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
  oled.print(" %");
  oled.display();

  delay(500);
}
