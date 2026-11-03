#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

String writeKey = "PASTE_YOUR_WRITE_KEY";

int dhtPin = 15;
int soilPin = 34;

DHT dht(dhtPin, DHT22);

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin("Wokwi-GUEST", "", 6);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println(" connected");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int soil = map(analogRead(soilPin), 0, 4095, 0, 100);

  String url = "http://api.thingspeak.com/update?api_key=" + writeKey;
  url = url + "&field1=" + String(temp, 1);
  url = url + "&field2=" + String(hum, 0);
  url = url + "&field3=" + String(soil);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  String answer = http.getString();
  http.end();

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("   Hum: ");
  Serial.print(hum);
  Serial.print("   Soil: ");
  Serial.print(soil);
  Serial.print("   HTTP ");
  Serial.print(code);
  Serial.print(", entry ");
  Serial.println(answer);

  delay(20000);
}
