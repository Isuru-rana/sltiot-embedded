// I3 ALERTS: I2 + a push notification to your phone (ntfy app)
// One push when a problem starts (HEAT, DOOR OPEN, SOIL DRY), one when it clears.
// Install the free ntfy app, subscribe to the same topic name as NTFY_TOPIC below.

#define USE_TLS 0

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <esp_task_wdt.h>

// ---- YOUR TURN: your greenhouse name, e.g. "KANDY-21" (place + number, no spaces) ----
const char* FARM_ID = "GREENHOUSE-01";

const char* MQTT_HOST = "broker.hivemq.com";   // public test broker: no private data!

// ---- YOUR TURN: a hard-to-guess topic name. Anyone who knows it gets your alerts. ----
const char* NTFY_TOPIC = "sltgh-greenhouse-01-change-me";

const int DHT_PIN = 15, SOIL_PIN = 34, DOOR_PIN = 27, BUTTON_PIN = 14;
const int FAN_PIN = 25, PUMP_PIN = 26, ALARM_PIN = 4;
const int DRY_BELOW = 30;
const float HOT_C = 40.0, FAN_START_C = 25.0, FAN_FULL_C = 35.0;
const unsigned long WATER_MS = 5000, SOAK_MS = 10000;

enum State { IDLE, WATERING, SOAKING, ALARM };
const char* NAMES[] = { "IDLE", "WATERING", "SOAKING", "ALARM" };
State state = IDLE;
unsigned long stateStart = 0;

#if USE_TLS
// Amazon Root CA 1: the certificate authority behind broker.hivemq.com
const char* ROOT_CA = R"(-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----)";
WiFiClientSecure net;
const int MQTT_PORT = 8883;
#else
WiFiClient net;
const int MQTT_PORT = 1883;
#endif

PubSubClient mqtt(net);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DHT dht(DHT_PIN, DHT22);

float temp = 25, hum = 60;
int soil = 50;
bool doorOpen = false, buttonWas = false, rain = false, fanMax = false, waterOrder = false;
String lastAlarms = "";
String tTelemetry, tStatus, tEvent, tCmd;
unsigned long lastSensor = 0, lastScreen = 0, lastSend = 0, lastTry = 0;

void enter(State s) { Serial.printf("%s -> %s\n", NAMES[state], NAMES[s]); state = s; stateStart = millis(); lastSend = 0; }
unsigned long inState() { return millis() - stateStart; }

int fanPercent() {
  if (state == ALARM || fanMax) return 100;
  if (doorOpen || temp <= FAN_START_C) return 0;
  if (temp >= FAN_FULL_C) return 100;
  return (temp - FAN_START_C) * 100 / (FAN_FULL_C - FAN_START_C);
}

String alarms() {
  String a;
  if (temp > HOT_C) a += "HEAT ";
  if (doorOpen) a += "DOOR OPEN ";
  if (soil < 15) a += "SOIL DRY ";
  a.trim();
  return a;
}

// Orders from the wall (this farm) or the region (all farms)
void onCommand(char* topic, byte* payload, unsigned int len) {
  String cmd;
  for (unsigned i = 0; i < len; i++) cmd += (char)payload[i];
  cmd.trim();
  Serial.printf("ORDER <- %s: %s\n", topic, cmd.c_str());
  if (cmd == "WATER") waterOrder = true;
  else if (cmd == "FAN_MAX") fanMax = true;
  else if (cmd == "AUTO") fanMax = false;
  else if (cmd == "RAIN_ON") rain = true;
  else if (cmd == "RAIN_OFF") rain = false;
  lastSend = 0;
}

void connectMqtt() {
  if (mqtt.connected() || millis() - lastTry < 5000) return;
  lastTry = millis();
  String id = String("gh-") + FARM_ID + "-" + String(random(0xffff), HEX);
  Serial.print("MQTT... ");
  // Last Will: if we vanish, the broker announces "offline" for us
  if (mqtt.connect(id.c_str(), tStatus.c_str(), 0, true, "offline")) {
    Serial.println("connected");
    mqtt.publish(tStatus.c_str(), "online", true);
    mqtt.subscribe(tCmd.c_str());
    mqtt.subscribe("sltgh/region/cmd");
    String why = esp_reset_reason() == ESP_RST_TASK_WDT ? "rebooted by WATCHDOG" : "started";
    mqtt.publish(tEvent.c_str(), (String(FARM_ID) + ": " + why).c_str());
  } else {
    Serial.printf("failed (%d), retry in 5 s\n", mqtt.state());
  }
}

void readSensors() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t)) { temp = t; hum = h; }
  soil = map(analogRead(SOIL_PIN), 0, 4095, 0, 100);
}

void publishTelemetry() {
  char json[320];
  snprintf(json, sizeof json,
    "{\"farm\":\"%s\",\"state\":\"%s\",\"mode\":\"%s\",\"rain\":%s,\"temp\":%.1f,\"hum\":%.0f,\"soil\":%d,"
    "\"fan\":%d,\"pump\":\"%s\",\"door\":\"%s\",\"alarms\":\"%s\",\"rssi\":%d}",
    FARM_ID, NAMES[state], fanMax ? "FAN_MAX" : "AUTO", rain ? "true" : "false", temp, hum, soil,
    fanPercent(), state == WATERING ? "ON" : "OFF", doorOpen ? "OPEN" : "CLOSED", alarms().c_str(), WiFi.RSSI());
  mqtt.publish(tTelemetry.c_str(), json);
  Serial.printf("SENT %s\n", json);
}

void sendPush(const String& title, const char* priority) {
  WiFiClientSecure tls;
  tls.setInsecure();   // workshop shortcut: skips the certificate check (a real product pins the CA)
  HTTPClient http;
  http.begin(tls, String("https://ntfy.sh/") + NTFY_TOPIC);
  http.addHeader("Title", title);
  http.addHeader("Priority", priority);
  http.addHeader("Tags", "seedling");
  int code = http.POST(String("Greenhouse ") + FARM_ID + ", temp " + String(temp, 1) + " C, soil " + soil + " %");
  http.end();
  Serial.printf("PUSH %s -> HTTP %d\n", title.c_str(), code);
}

void publishEvents() {
  String now = alarms();
  const char* watch[] = { "HEAT", "DOOR OPEN", "SOIL DRY" };
  for (const char* a : watch) {
    bool on = now.indexOf(a) >= 0, before = lastAlarms.indexOf(a) >= 0;
    if (on != before) {
      String msg = String(FARM_ID) + ": " + a + (on ? " RAISED" : " cleared");
      mqtt.publish(tEvent.c_str(), msg.c_str());
      Serial.println("EVENT " + msg);
      sendPush(msg, on ? (strcmp(a, "HEAT") == 0 ? "urgent" : "high") : "default");
    }
  }
  lastAlarms = now;
}

void showScreen() {
  oled.clearDisplay();
  oled.setTextSize(1); oled.setCursor(0, 0);
  oled.printf("%s %s\n", mqtt.connected() ? "ONLINE" : "offline", rain ? "RAIN" : "");
  oled.setTextSize(2); oled.setCursor(0, 14); oled.println(NAMES[state]);
  oled.setTextSize(1); oled.setCursor(0, 38);
  oled.printf("%.1f C  %.0f %%RH\nSoil %d %%  Fan %d %%", temp, hum, soil, fanPercent());
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);          // safe default: pump OFF
  pinMode(ALARM_PIN, OUTPUT);
  ledcAttach(FAN_PIN, 5000, 8);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);

  tTelemetry = String("sltgh/") + FARM_ID + "/telemetry";
  tStatus = String("sltgh/") + FARM_ID + "/status";
  tEvent = String("sltgh/") + FARM_ID + "/event";
  tCmd = String("sltgh/") + FARM_ID + "/cmd";

  Serial.print("WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf(" connected, IP %s\n", WiFi.localIP().toString().c_str());
#if USE_TLS
  net.setCACert(ROOT_CA);
#endif
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onCommand);
  mqtt.setSocketTimeout(5);

  esp_task_wdt_config_t cfg = { .timeout_ms = 20000, .idle_core_mask = 0, .trigger_panic = true };
  if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) esp_task_wdt_init(&cfg);
  esp_task_wdt_add(NULL);
  readSensors();
  Serial.printf("I3 ready as %s (%s)\n", FARM_ID, USE_TLS ? "TLS 8883" : "plain 1883");
}

void loop() {
  esp_task_wdt_reset();
  connectMqtt();
  mqtt.loop();

  if (millis() - lastSensor >= 2000) { lastSensor = millis(); readSensors(); }
  doorOpen = digitalRead(DOOR_PIN) == HIGH;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  bool newPress = pressed && !buttonWas;
  buttonWas = pressed;

  switch (state) {
    case IDLE:
      if (temp > HOT_C) enter(ALARM);
      else if ((soil < DRY_BELOW && !rain) || newPress || waterOrder) { waterOrder = false; enter(WATERING); }
      break;
    case WATERING:
      if (temp > HOT_C) enter(ALARM);
      else if (inState() >= WATER_MS) enter(SOAKING);
      break;
    case SOAKING:
      if (inState() >= SOAK_MS) enter(IDLE);
      break;
    case ALARM:
      if (temp < HOT_C - 2) enter(IDLE);
      break;
  }

  digitalWrite(PUMP_PIN, state == WATERING);
  ledcWrite(FAN_PIN, fanPercent() * 255 / 100);
  bool blink = (millis() / 300) % 2;
  digitalWrite(ALARM_PIN, state == ALARM ? blink : doorOpen);

  if (mqtt.connected()) {
    publishEvents();
    if (millis() - lastSend >= 5000) { lastSend = millis(); publishTelemetry(); }
  }
  if (millis() - lastScreen >= 500) { lastScreen = millis(); showScreen(); }
}
