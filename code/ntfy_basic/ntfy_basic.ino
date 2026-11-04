#include <WiFi.h>
#include <HTTPClient.h>

String topic = "sltiot-demo-change-me";
int count = 0;

void setup() {
  Serial.begin(115200);

  Serial.print("Connecting to WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi connected");
}

void loop() {
  if (count < 10) {
    count = count + 1;

    HTTPClient http;
    http.begin("http://ntfy.sh/" + topic);
    int code = http.POST("Hello from ESP32. Message " + String(count));

    Serial.print("Sent message ");
    Serial.print(count);
    Serial.print("  HTTP code: ");
    Serial.println(code);

    http.end();
  }

  delay(5000);
}
