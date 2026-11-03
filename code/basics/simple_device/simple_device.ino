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

void setup() {
  Serial.begin(115200);

  pinMode(doorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);

  dht.begin();

  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);
}

void loop() {
  float temp;
  temp = dht.readTemperature();

  int soilRaw;
  soilRaw = analogRead(soilPin);

  int soil;
  soil = soilRaw * 100 / 4095;

  int door;
  door = digitalRead(doorPin);

  int button;
  button = digitalRead(buttonPin);

  int fan;

  if (temp > 35) {
    fan = 255;
  } else if (temp > 28) {
    fan = 120;
  } else {
    fan = 0;
  }

  analogWrite(fanPin, fan);

  if (soil < 30 || button == LOW) {
    digitalWrite(pumpPin, HIGH);
  } else {
    digitalWrite(pumpPin, LOW);
  }

  if (door == HIGH || temp > 40) {
    digitalWrite(alarmPin, HIGH);
  } else {
    digitalWrite(alarmPin, LOW);
  }

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("  Soil: ");
  Serial.print(soil);
  Serial.print("  Fan: ");
  Serial.println(fan);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("GREENHOUSE");

  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.print(temp, 1);
  oled.print(" C");

  oled.setCursor(0, 40);
  oled.print("Soil ");
  oled.print(soil);
  oled.print("%");

  oled.display();

  delay(500);
}
