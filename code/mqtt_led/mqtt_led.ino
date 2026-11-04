#include <WiFi.h>
#include <PubSubClient.h>

int ledPin = 4;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void received(char* topic, byte* payload, unsigned int length) {
  if (payload[0] == '1') {
    digitalWrite(ledPin, HIGH);
    Serial.println("LED ON");
  }

  if (payload[0] == '0') {
    digitalWrite(ledPin, LOW);
    Serial.println("LED OFF");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected");

  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setCallback(received);
  while (mqtt.connect(String("sltiot-" + String(random(100000))).c_str()) == false) {
    delay(1000);
  }
  mqtt.subscribe("sltiot/change-me/led");
  Serial.println("MQTT connected. Send 1 or 0 to sltiot/change-me/led");
}

void loop() {
  mqtt.loop();
}
