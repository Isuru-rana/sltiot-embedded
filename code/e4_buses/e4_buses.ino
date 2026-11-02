// E4 BUSES: sensors and screens share wires
// Step 1: scan the I2C bus and list every address that answers.
// Step 2: show the greenhouse on the OLED screen (I2C, address 0x3C).
// The DHT22 uses its own one-wire style protocol on pin 15.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

const int SDA_PIN = 21, SCL_PIN = 22;
const int DHT_PIN = 15, SOIL_PIN = 34;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DHT dht(DHT_PIN, DHT22);

void scanI2C() {
  Serial.println("Scanning I2C bus...");
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  device at 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("%d device(s) found\n", found);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  dht.begin();
  scanI2C();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true) delay(1000);
  }
  oled.setTextColor(SSD1306_WHITE);
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil = map(analogRead(SOIL_PIN), 0, 4095, 0, 100);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("GREENHOUSE 01");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.printf("%.1f C\n", t);
  oled.printf("%.0f %% RH\n", h);
  oled.setTextSize(1);
  oled.printf("Soil %d %%", soil);
  oled.display();

  Serial.printf("temp=%.1f  hum=%.0f  soil=%d\n", t, h, soil);
  delay(2000);
}
