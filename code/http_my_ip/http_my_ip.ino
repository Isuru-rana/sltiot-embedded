#include <WiFi.h>
#include <HTTPClient.h>

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
  HTTPClient http;
  http.begin("http://api.ipify.org");
  int code = http.GET();

  Serial.print("HTTP code: ");
  Serial.println(code);

  if (code == 200) {
    String data = http.getString();
    data.trim();
    Serial.print("My internet IP: ");
    Serial.println(data);
  }

  http.end();
  delay(10000);
}
