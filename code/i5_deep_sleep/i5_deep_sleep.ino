// I5 DEEP SLEEP: a battery soil sensor out in the field
// Wake, measure, send one message, sleep. The ESP32 is awake about 2 to 4 s per cycle.
// Asleep it uses about 10 microamps, awake about 100 mA. That is the whole battery-life secret.
// RTC memory survives deep sleep, normal RAM does not.
// Wokwi note: the simulator restarts the chip instead of a true deep sleep, so the wake
// counter shows 1 every time there. On a real ESP32 it counts 1, 2, 3...

#include <WiFi.h>
#include <PubSubClient.h>

// ---- YOUR TURN: your sensor name ----
const char* NODE_ID = "FIELD-01";
const int SLEEP_S = 20;               // a real field sensor sleeps 15 to 60 minutes
const int SOIL_PIN = 34;

RTC_DATA_ATTR int bootCount = 0;      // kept in RTC memory across deep sleep
RTC_DATA_ATTR int lastSoil = -1;

WiFiClient net;
PubSubClient mqtt(net);

void setup() {
  unsigned long t0 = millis();
  Serial.begin(115200);
  bootCount++;
  int soil = map(analogRead(SOIL_PIN), 0, 4095, 0, 100);
  Serial.printf("\nWake #%d (reason %d). Soil %d %% (last time %d %%)\n",
                bootCount, esp_sleep_get_wakeup_cause(), soil, lastSoil);
  lastSoil = soil;

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) delay(100);
  mqtt.setServer("broker.hivemq.com", 1883);
  String id = String("node-") + NODE_ID + "-" + String(random(0xffff), HEX);
  if (WiFi.status() == WL_CONNECTED && mqtt.connect(id.c_str())) {
    char json[120];
    snprintf(json, sizeof json, "{\"node\":\"%s\",\"soil\":%d,\"wake\":%d}", NODE_ID, soil, bootCount);
    mqtt.publish((String("sltgh/") + NODE_ID + "/telemetry").c_str(), json);
    mqtt.loop();
    delay(100);                        // let the message leave before the radio switches off
    Serial.printf("SENT %s\n", json);
  } else {
    Serial.println("No network this time. Try again next wake.");
  }

  Serial.printf("Awake for %lu ms. Sleeping %d s...\n", millis() - t0, SLEEP_S);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_S * 1000000ULL);
  esp_deep_sleep_start();              // never returns: next wake starts setup() again
}

void loop() { }                        // never reached
