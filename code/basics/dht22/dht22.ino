#include <DHT.h>

DHT dht(15, DHT22);

void setup() {
  Serial.begin(115200);

  dht.begin();
}

void loop() {

  float temp;
  float humid;

  temp = dht.readTemperature();
  humid = dht.readHumidity();

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("'C  humid:  ");
  Serial.println(humid);
}
