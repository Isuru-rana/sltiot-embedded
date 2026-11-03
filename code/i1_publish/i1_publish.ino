#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

String farmId = "GREENHOUSE-01";

int dhtPin = 15;
int soilPin = 34;

DHT dht(dhtPin, DHT22);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastSendTime = 0;

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
  mqtt.setServer("broker.hivemq.com", 1883);
}

void loop() {
  if (mqtt.connected() == false) {
    Serial.print("Connecting to MQTT... ");
    String clientId = "gh-" + farmId + "-" + String(random(100000));

    if (mqtt.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.println("failed, try again");
      delay(2000);
    }
  }

  mqtt.loop();

  if (millis() - lastSendTime >= 5000) {
    lastSendTime = millis();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int soil = map(analogRead(soilPin), 0, 4095, 0, 100);

    String json = "{";
    json = json + "\"farm\":\"" + farmId + "\",";
    json = json + "\"temp\":" + String(temp, 1) + ",";
    json = json + "\"hum\":" + String(hum, 0) + ",";
    json = json + "\"soil\":" + String(soil) + ",";
    json = json + "\"rssi\":" + String(WiFi.RSSI());
    json = json + "}";

    String topic = "sltgh/" + farmId + "/telemetry";
    mqtt.publish(topic.c_str(), json.c_str());
    Serial.println("SENT " + json);
  }
}
