// I4 CLOUD PLATFORM: send readings to ThingSpeak and get free charts
// HTTP instead of MQTT: one web request every 20 s (free plan: at most 1 every 15 s).
// 1. Make a free account at thingspeak.com, create a channel with 3 fields:
//    Field 1 Temperature, Field 2 Humidity, Field 3 Soil
// 2. Copy the channel's Write API Key below. Make the channel Public to share the charts.

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ---- YOUR TURN: your channel's Write API Key ----
const char* WRITE_KEY = "PASTE_YOUR_WRITE_KEY";

const int DHT_PIN = 15, SOIL_PIN = 34;
DHT dht(DHT_PIN, DHT22);
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.print("WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.println(" connected");
}

void loop() {
  if (millis() - lastSend < 20000 && lastSend != 0) return;
  lastSend = millis();

  float t = dht.readTemperature(), h = dht.readHumidity();
  int soil = map(analogRead(SOIL_PIN), 0, 4095, 0, 100);

  String url = String("http://api.thingspeak.com/update?api_key=") + WRITE_KEY +
               "&field1=" + String(t, 1) + "&field2=" + String(h, 0) + "&field3=" + soil;
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  String body = http.getString();     // ThingSpeak answers with the entry number, 0 = rejected
  http.end();
  Serial.printf("temp=%.1f hum=%.0f soil=%d  -> HTTP %d, entry %s\n", t, h, soil, code, body.c_str());
}
