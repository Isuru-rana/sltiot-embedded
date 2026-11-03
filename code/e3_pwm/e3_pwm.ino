#include <DHT.h>

int dhtPin = 15;
int fanPin = 25;

DHT dht(dhtPin, DHT22);

void setup() {
  Serial.begin(115200);
  dht.begin();
  ledcAttach(fanPin, 5000, 8);
  Serial.println("E3 ready. Click the DHT22 and change the temperature.");
}

void loop() {
  float temp = dht.readTemperature();

  int fanSpeed = map(temp, 25, 35, 0, 255);

  if (fanSpeed < 0) {
    fanSpeed = 0;
  }

  if (fanSpeed > 255) {
    fanSpeed = 255;
  }

  ledcWrite(fanPin, fanSpeed);

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" C   Fan speed: ");
  Serial.print(fanSpeed);
  Serial.println(" / 255");

  delay(2000);
}
