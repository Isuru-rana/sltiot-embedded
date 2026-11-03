#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

void setup() {
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  oled.setTextColor(SSD1306_WHITE);
}

void loop() {
  oled.clearDisplay();

  oled.setTextSize(1);
  oled.setCursor(10, 20);
  oled.print("SLT IOT!");

  oled.display();

}
