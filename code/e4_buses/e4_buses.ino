#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

int dhtPin = 15;
int soilPin = 34;

DHT dht(dhtPin, DHT22);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();

  Serial.println("Scanning I2C bus...");

  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {
      Serial.print("Device found at 0x");
      Serial.println(address, HEX);
    }
  }

  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int soil = map(analogRead(soilPin), 0, 4095, 0, 100);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("GREENHOUSE 01");

  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.print(temp, 1);
  oled.println(" C");
  oled.print(hum, 0);
  oled.println(" %RH");

  oled.setTextSize(1);
  oled.print("Soil ");
  oled.print(soil);
  oled.print(" %");
  oled.display();

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("   Hum: ");
  Serial.print(hum);
  Serial.print("   Soil: ");
  Serial.println(soil);

  delay(2000);
}
