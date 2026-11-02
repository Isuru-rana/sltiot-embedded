// FARM SIMULATOR: one ESP32 pretends to be 15 greenhouses
// Keeps the Greenhouse Control Wall alive. Runs in its own Wokwi tab (keep that tab visible).
// Same topics and JSON as the real I2 greenhouse, plus "sim": true so the wall can hide them.
// One simulated day = 5 minutes. Scripted story every day:
//   NUWARAELIYA-03 overheats in the afternoon (HEAT alarm)
//   BANDARAWELA-07 dries out fast and waters often
//   WELIMADA-11 leaves its door open for a while (DOOR OPEN)
//   HAPUTALE-05 has a broken pump: it waters but the soil stays dry (PUMP FAULT)
//   DAMBULLA-02 drops offline for a minute, then comes back

#include <WiFi.h>
#include <PubSubClient.h>

const char* BASE = "sltgh";
const float DAY_S = 300;
const int N = 15;
const char* NAMES[N] = { "NUWARAELIYA-03", "BANDARAWELA-07", "WELIMADA-11", "HAPUTALE-05", "DAMBULLA-02",
  "KANDAPOLA-04", "RAGALA-09", "BADULLA-12", "KEPPETIPOLA-06", "MATALE-08",
  "KANDY-15", "KURUNEGALA-10", "ANURADHAPURA-01", "JAFFNA-13", "HAMBANTOTA-14" };
enum { HOT = 0, THIRSTY = 1, DOOR = 2, BROKEN = 3, FLAKY = 4 };
const char* STATES[] = { "IDLE", "WATERING", "SOAKING", "ALARM" };

struct Farm {
  float soil, use, baseTemp, temp; int hum, rssi;
  int state; unsigned long stateAt; int mode;   // mode 0 AUTO, 1 FAN_MAX
  bool forceWater, door, offline; String lastAlarms;
};
Farm farms[N];
bool rain = false;

WiFiClient net;
PubSubClient mqtt(net);
unsigned long t0;

void onCommand(char* topic, byte* payload, unsigned int len) {
  String cmd; for (unsigned i = 0; i < len; i++) cmd += (char)payload[i];
  String t = topic;
  if (t.endsWith("/region/cmd")) { rain = (cmd == "RAIN_ON"); Serial.println("REGION " + cmd); return; }
  for (int i = 0; i < N; i++) if (t == String(BASE) + "/" + NAMES[i] + "/cmd") {
    if (cmd == "WATER") farms[i].forceWater = true;
    else if (cmd == "FAN_MAX") farms[i].mode = 1;
    else if (cmd == "AUTO") farms[i].mode = 0;
    Serial.printf("ORDER %s %s\n", NAMES[i], cmd.c_str());
  }
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.print("MQTT... ");
    if (mqtt.connect(("farmsim-" + String(random(0xffff), HEX)).c_str())) {
      Serial.println("connected");
      mqtt.subscribe("sltgh/region/cmd");
      for (int i = 0; i < N; i++) mqtt.subscribe((String(BASE) + "/" + NAMES[i] + "/cmd").c_str());
    } else { Serial.println("retry"); delay(2000); }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(7);
  for (int i = 0; i < N; i++) {
    farms[i] = { (float)random(40, 80), random(4, 12) / 10.0f, random(170, 220) / 10.0f, 25, random(60, 85), (int)random(-85, -55),
                 0, 0, 0, false, false, false, "" };
  }
  farms[THIRSTY].use = 2.5;
  farms[BROKEN].soil = 35;
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) delay(250);
  Serial.println("WiFi connected");
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onCommand);
  connectMqtt();
  t0 = millis();
  Serial.printf("Farm simulator running: %d greenhouses\n", N);
}

void stepFarm(int i, float phase, float sun) {
  Farm& f = farms[i];
  unsigned long now = millis();
  f.temp = f.baseTemp + 9 * sun + random(-3, 4) / 10.0f;
  if (i == HOT) f.temp += 14 * max(0.0f, sinf(PI * constrain((phase - 0.05f) / 0.35f, 0.0f, 1.0f)));
  f.door = (i == DOOR) && phase > 0.15 && phase < 0.30;
  f.soil = max(0.0f, f.soil - f.use * (0.5f + sun) * random(5, 15) / 10.0f);

  bool hot = f.temp > 40;
  switch (f.state) {
    case 0: if (hot) f.state = 3, f.stateAt = now;
            else if ((f.soil < 30 && !rain) || f.forceWater) f.state = 1, f.stateAt = now, f.forceWater = false;
            break;
    case 1: if (hot) f.state = 3, f.stateAt = now; else if (now - f.stateAt > 10000) f.state = 2, f.stateAt = now; break;
    case 2: if (now - f.stateAt > 15000) f.state = 0, f.stateAt = now; break;
    case 3: if (f.temp < 38) f.state = 0, f.stateAt = now; break;
  }
  if (f.state == 1 && i != BROKEN) f.soil = min(100.0f, f.soil + 9);

  int fan = f.state == 3 || f.mode == 1 ? 100 : (f.door ? 0 : constrain((int)((f.temp - 25) * 10), 0, 100));
  String alarms;
  if (hot) alarms += "HEAT ";
  if (f.door) alarms += "DOOR OPEN ";
  if (i == BROKEN && f.soil < 15) alarms += "PUMP FAULT ";
  else if (f.soil < 15) alarms += "SOIL DRY ";
  alarms.trim();

  char json[400];
  snprintf(json, sizeof json,
    "{\"farm\":\"%s\",\"state\":\"%s\",\"mode\":\"%s\",\"rain\":%s,\"temp\":%.1f,\"hum\":%d,\"soil\":%d,"
    "\"fan\":%d,\"pump\":\"%s\",\"door\":\"%s\",\"alarms\":\"%s\",\"rssi\":%d,\"sim\":true}",
    NAMES[i], STATES[f.state], f.mode ? "FAN_MAX" : "AUTO", rain ? "true" : "false", f.temp, f.hum, (int)f.soil,
    fan, f.state == 1 ? "ON" : "OFF", f.door ? "OPEN" : "CLOSED", alarms.c_str(), f.rssi + (int)random(-4, 5));
  mqtt.publish((String(BASE) + "/" + NAMES[i] + "/telemetry").c_str(), json);

  const char* watch[] = { "HEAT", "DOOR OPEN", "PUMP FAULT", "SOIL DRY" };
  for (const char* a : watch) {
    bool now_ = alarms.indexOf(a) >= 0, before = f.lastAlarms.indexOf(a) >= 0;
    if (now_ != before) mqtt.publish((String(BASE) + "/" + NAMES[i] + "/event").c_str(),
                                     (String(NAMES[i]) + ": " + a + (now_ ? " RAISED" : " cleared")).c_str());
  }
  f.lastAlarms = alarms;
}

void loop() {
  connectMqtt();
  float phase = fmodf((millis() - t0) / 1000.0f, DAY_S) / DAY_S;     // 0 = morning
  float sun = max(0.0f, sinf(2 * PI * phase));                      // 0 at night
  Farm& fl = farms[FLAKY];
  bool shouldBeOff = phase > 0.55 && phase < 0.75;
  if (shouldBeOff != fl.offline) {
    fl.offline = shouldBeOff;
    mqtt.publish((String(BASE) + "/" + NAMES[FLAKY] + "/status").c_str(), shouldBeOff ? "offline" : "online");
  }
  for (int i = 0; i < N; i++) {
    if (farms[i].offline) continue;
    stepFarm(i, phase, sun);
    for (int k = 0; k < 10; k++) { mqtt.loop(); delay(20); }
  }
  Serial.printf("day %.0f %%  sun %.2f  rain %s\n", phase * 100, sun, rain ? "yes" : "no");
  for (int k = 0; k < 100; k++) { mqtt.loop(); delay(20); }
}
