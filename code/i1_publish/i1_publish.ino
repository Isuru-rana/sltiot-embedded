// I1 PUBLISH: the greenhouse sends its numbers to the internet
// WiFi + MQTT. Every 5 s one JSON message goes to sltgh/<FARM>/telemetry.
// Watch it in the HiveMQ web client (subscribe to sltgh/#) or on the wall.

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ---- YOUR TURN: your greenhouse name, e.g. "KANDY-21" (place + number, no spaces) ----
const char* FARM_ID = "GREENHOUSE-01";

const char* MQTT_HOST = "broker.hivemq.com";   // public test broker: no private data!
const int DHT_PIN = 15, SOIL_PIN = 34;

DHT dht(DHT_PIN, DHT22);
WiFiClient net;
PubSubClient mqtt(net);
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.print("WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6);          // channel 6 skips the scan
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf(" connected, IP %s\n", WiFi.localIP().toString().c_str());
  mqtt.setServer(MQTT_HOST, 1883);
}

void loop() {
  if (!mqtt.connected()) {
    Serial.print("MQTT... ");
    String id = String("gh-") + FARM_ID + "-" + String(random(0xffff), HEX);
    Serial.println(mqtt.connect(id.c_str()) ? "connected" : "failed, retry");
    if (!mqtt.connected()) { delay(2000); return; }
  }
  mqtt.loop();

  if (millis() - lastSend >= 5000) {
    lastSend = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    int soil = map(analogRead(SOIL_PIN), 0, 4095, 0, 100);

    char json[160];
    snprintf(json, sizeof json, "{\"farm\":\"%s\",\"temp\":%.1f,\"hum\":%.0f,\"soil\":%d,\"rssi\":%d}",
             FARM_ID, t, h, soil, WiFi.RSSI());
    String topic = String("sltgh/") + FARM_ID + "/telemetry";
    mqtt.publish(topic.c_str(), json);
    Serial.printf("SENT %s  %s\n", topic.c_str(), json);
  }
}
