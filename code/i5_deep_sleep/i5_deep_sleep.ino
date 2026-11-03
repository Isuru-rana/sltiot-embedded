#include <WiFi.h>
#include <PubSubClient.h>

String nodeId = "FIELD-01";

int soilPin = 34;
int sleepSeconds = 20;

RTC_DATA_ATTR int wakeCount = 0;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() {
  Serial.begin(115200);
  wakeCount++;

  int soil = map(analogRead(soilPin), 0, 4095, 0, 100);

  Serial.println();
  Serial.print("Wake number ");
  Serial.print(wakeCount);
  Serial.print(". Soil ");
  Serial.print(soil);
  Serial.println(" %");

  WiFi.begin("Wokwi-GUEST", "", 6);

  while (WiFi.status() != WL_CONNECTED && millis() < 20000) {
    delay(100);
  }

  mqtt.setServer("broker.hivemq.com", 1883);
  String clientId = "node-" + nodeId + "-" + String(random(100000));

  if (WiFi.status() == WL_CONNECTED && mqtt.connect(clientId.c_str())) {
    String json = "{";
    json = json + "\"node\":\"" + nodeId + "\",";
    json = json + "\"soil\":" + String(soil) + ",";
    json = json + "\"wake\":" + String(wakeCount);
    json = json + "}";

    String topic = "sltgh/" + nodeId + "/telemetry";
    mqtt.publish(topic.c_str(), json.c_str());
    mqtt.loop();
    delay(100);
    Serial.println("SENT " + json);
  } else {
    Serial.println("No network this time. Try again next wake.");
  }

  Serial.print("Awake for ");
  Serial.print(millis());
  Serial.print(" ms. Sleeping ");
  Serial.print(sleepSeconds);
  Serial.println(" s...");
  Serial.flush();

  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
}
