#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() {
  Serial.begin(115200);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected");

  mqtt.setServer("broker.hivemq.com", 1883);
  while (mqtt.connect(String("sltiot-" + String(random(100000))).c_str()) == false) {
    delay(1000);
  }
  Serial.println("MQTT connected");
}

void loop() {
  mqtt.publish("sltiot/change-me/hello", "Hello from ESP32");
  Serial.println("Message sent");

  mqtt.loop();
  delay(5000);
}
