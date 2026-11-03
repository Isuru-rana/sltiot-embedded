#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <esp_task_wdt.h>
#include <HTTPClient.h>
String farmId = "GREENHOUSE-01";
String ntfyTopic = "sltgh-greenhouse-01-change-me";
bool useTLS = false;

int dhtPin = 15;
int soilPin = 34;
int doorPin = 27;
int buttonPin = 14;
int fanPin = 25;
int pumpPin = 26;
int alarmPin = 4;

const char* rootCA = R"(-----BEGIN CERTIFICATE-----
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

WiFiClient plainClient;
WiFiClientSecure secureClient;
PubSubClient mqtt;
DHT dht(dhtPin, DHT22);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

String telemetryTopic = "sltgh/" + farmId + "/telemetry";
String statusTopic = "sltgh/" + farmId + "/status";
String eventTopic = "sltgh/" + farmId + "/event";
String commandTopic = "sltgh/" + farmId + "/cmd";

String state = "IDLE";
unsigned long stateStartTime = 0;

float temp = 25;
float hum = 60;
int soil = 50;
bool doorOpen = false;
int lastButton = HIGH;

bool rain = false;
bool fanMax = false;
bool waterOrder = false;

bool heatAlarm = false;
bool doorAlarm = false;
bool dryAlarm = false;
bool lastHeatAlarm = false;
bool lastDoorAlarm = false;
bool lastDryAlarm = false;

unsigned long lastSensorTime = 0;
unsigned long lastScreenTime = 0;
unsigned long lastSendTime = 0;
unsigned long lastTryTime = 0;

void changeState(String newState) {
  Serial.print(state);
  Serial.print(" to ");
  Serial.println(newState);
  state = newState;
  stateStartTime = millis();
  lastSendTime = 0;
}

int fanPercent() {
  if (state == "ALARM" || fanMax == true) {
    return 100;
  }

  if (doorOpen == true) {
    return 0;
  }

  int percent = map(temp, 25, 35, 0, 100);

  if (percent < 0) {
    percent = 0;
  }

  if (percent > 100) {
    percent = 100;
  }

  return percent;
}

String alarmText() {
  String text = "";

  if (heatAlarm == true) {
    text = text + "HEAT ";
  }

  if (doorAlarm == true) {
    text = text + "DOOR OPEN ";
  }

  if (dryAlarm == true) {
    text = text + "SOIL DRY ";
  }

  text.trim();
  return text;
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String command = "";

  for (int i = 0; i < length; i++) {
    command = command + (char)payload[i];
  }

  command.trim();
  Serial.print("ORDER: ");
  Serial.println(command);

  if (command == "WATER") {
    waterOrder = true;
  } else if (command == "FAN_MAX") {
    fanMax = true;
  } else if (command == "AUTO") {
    fanMax = false;
  } else if (command == "RAIN_ON") {
    rain = true;
  } else if (command == "RAIN_OFF") {
    rain = false;
  }

  lastSendTime = 0;
}

void connectMqtt() {
  lastTryTime = millis();
  String clientId = "gh-" + farmId + "-" + String(random(100000));
  Serial.print("Connecting to MQTT... ");

  if (mqtt.connect(clientId.c_str(), statusTopic.c_str(), 0, true, "offline")) {
    Serial.println("connected");
    mqtt.publish(statusTopic.c_str(), "online", true);
    mqtt.subscribe(commandTopic.c_str());
    mqtt.subscribe("sltgh/region/cmd");

    String message = farmId + ": started";

    if (esp_reset_reason() == ESP_RST_TASK_WDT) {
      message = farmId + ": rebooted by WATCHDOG";
    }

    mqtt.publish(eventTopic.c_str(), message.c_str());
  } else {
    Serial.println("failed, try again in 5 s");
  }
}

void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) {
    temp = t;
    hum = h;
  }

  soil = map(analogRead(soilPin), 0, 4095, 0, 100);

  if (temp > 40) {
    heatAlarm = true;
  } else {
    heatAlarm = false;
  }

  if (soil < 15) {
    dryAlarm = true;
  } else {
    dryAlarm = false;
  }
}

void sendTelemetry() {
  String mode = "AUTO";

  if (fanMax == true) {
    mode = "FAN_MAX";
  }

  String rainText = "false";

  if (rain == true) {
    rainText = "true";
  }

  String pump = "OFF";

  if (state == "WATERING") {
    pump = "ON";
  }

  String door = "CLOSED";

  if (doorOpen == true) {
    door = "OPEN";
  }

  String json = "{";
  json = json + "\"farm\":\"" + farmId + "\",";
  json = json + "\"state\":\"" + state + "\",";
  json = json + "\"mode\":\"" + mode + "\",";
  json = json + "\"rain\":" + rainText + ",";
  json = json + "\"temp\":" + String(temp, 1) + ",";
  json = json + "\"hum\":" + String(hum, 0) + ",";
  json = json + "\"soil\":" + String(soil) + ",";
  json = json + "\"fan\":" + String(fanPercent()) + ",";
  json = json + "\"pump\":\"" + pump + "\",";
  json = json + "\"door\":\"" + door + "\",";
  json = json + "\"alarms\":\"" + alarmText() + "\",";
  json = json + "\"rssi\":" + String(WiFi.RSSI());
  json = json + "}";

  mqtt.publish(telemetryTopic.c_str(), json.c_str());
  Serial.println("SENT " + json);
}

void sendPush(String title, String priority) {
  WiFiClientSecure pushClient;
  pushClient.setInsecure();

  HTTPClient http;
  http.begin(pushClient, "https://ntfy.sh/" + ntfyTopic);
  http.addHeader("Title", title);
  http.addHeader("Priority", priority);
  http.addHeader("Tags", "seedling");

  String text = "Greenhouse " + farmId + ", temp " + String(temp, 1) + " C, soil " + String(soil) + " %";
  int code = http.POST(text);
  http.end();

  Serial.print("PUSH ");
  Serial.print(title);
  Serial.print(" HTTP ");
  Serial.println(code);
}
void sendEvent(String name, bool isOn) {
  String message = farmId + ": " + name;

  if (isOn == true) {
    message = message + " RAISED";
  } else {
    message = message + " cleared";
  }

  mqtt.publish(eventTopic.c_str(), message.c_str());
  Serial.println("EVENT " + message);

  if (isOn == false) {
    sendPush(message, "default");
  } else if (name == "HEAT") {
    sendPush(message, "urgent");
  } else {
    sendPush(message, "high");
  }
}

void checkAlarms() {
  if (heatAlarm != lastHeatAlarm) {
    sendEvent("HEAT", heatAlarm);
    lastHeatAlarm = heatAlarm;
  }

  if (doorAlarm != lastDoorAlarm) {
    sendEvent("DOOR OPEN", doorAlarm);
    lastDoorAlarm = doorAlarm;
  }

  if (dryAlarm != lastDryAlarm) {
    sendEvent("SOIL DRY", dryAlarm);
    lastDryAlarm = dryAlarm;
  }
}

void showScreen() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);

  if (mqtt.connected()) {
    oled.print("ONLINE");
  } else {
    oled.print("offline");
  }

  if (rain == true) {
    oled.print("  RAIN");
  }

  oled.setTextSize(2);
  oled.setCursor(0, 14);
  oled.println(state);

  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.print(temp, 1);
  oled.print(" C  ");
  oled.print(hum, 0);
  oled.println(" %RH");
  oled.print("Soil ");
  oled.print(soil);
  oled.print(" %  Fan ");
  oled.print(fanPercent());
  oled.print(" %");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(doorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);
  ledcAttach(fanPin, 5000, 8);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);

  WiFi.begin("Wokwi-GUEST", "", 6);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println(" connected");

  if (useTLS == true) {
    secureClient.setCACert(rootCA);
    mqtt.setClient(secureClient);
    mqtt.setServer("broker.hivemq.com", 8883);
  } else {
    mqtt.setClient(plainClient);
    mqtt.setServer("broker.hivemq.com", 1883);
  }

  mqtt.setBufferSize(512);
  mqtt.setCallback(onMessage);
  mqtt.setSocketTimeout(5);

  esp_task_wdt_config_t config = { .timeout_ms = 20000, .idle_core_mask = 0, .trigger_panic = true };
  esp_task_wdt_reconfigure(&config);
  esp_task_wdt_add(NULL);

  readSensors();
  Serial.println("I3 ready as " + farmId);
}

void loop() {
  esp_task_wdt_reset();

  if (mqtt.connected() == false && millis() - lastTryTime >= 5000) {
    connectMqtt();
  }

  mqtt.loop();

  if (millis() - lastSensorTime >= 2000) {
    lastSensorTime = millis();
    readSensors();
  }

  if (digitalRead(doorPin) == HIGH) {
    doorOpen = true;
  } else {
    doorOpen = false;
  }

  doorAlarm = doorOpen;

  bool pressed = false;
  int button = digitalRead(buttonPin);

  if (button == LOW && lastButton == HIGH) {
    pressed = true;
  }

  lastButton = button;

  unsigned long timeInState = millis() - stateStartTime;

  if (state == "IDLE") {
    if (temp > 40) {
      changeState("ALARM");
    } else if (pressed == true || waterOrder == true) {
      waterOrder = false;
      changeState("WATERING");
    } else if (soil < 30 && rain == false) {
      changeState("WATERING");
    }
  } else if (state == "WATERING") {
    if (temp > 40) {
      changeState("ALARM");
    } else if (timeInState >= 5000) {
      changeState("SOAKING");
    }
  } else if (state == "SOAKING") {
    if (timeInState >= 10000) {
      changeState("IDLE");
    }
  } else if (state == "ALARM") {
    if (temp < 38) {
      changeState("IDLE");
    }
  }

  if (state == "WATERING") {
    digitalWrite(pumpPin, HIGH);
  } else {
    digitalWrite(pumpPin, LOW);
  }

  ledcWrite(fanPin, fanPercent() * 255 / 100);

  if (state == "ALARM") {
    if ((millis() / 300) % 2 == 0) {
      digitalWrite(alarmPin, HIGH);
    } else {
      digitalWrite(alarmPin, LOW);
    }
  } else if (doorOpen == true) {
    digitalWrite(alarmPin, HIGH);
  } else {
    digitalWrite(alarmPin, LOW);
  }

  if (mqtt.connected()) {
    checkAlarms();

    if (millis() - lastSendTime >= 5000) {
      lastSendTime = millis();
      sendTelemetry();
    }
  }

  if (millis() - lastScreenTime >= 500) {
    lastScreenTime = millis();
    showScreen();
  }
}
