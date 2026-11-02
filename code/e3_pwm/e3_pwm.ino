// E3 ANALOG OUT (PWM): fan speed follows temperature
// PWM switches the pin on and off very fast. The share of ON time (duty)
// sets the average power. The FAN LED gets brighter as the fan speeds up.

#include <DHT.h>

const int DHT_PIN = 15;
const int FAN_PIN = 25;

const int PWM_FREQ = 5000;    // Hz
const int PWM_BITS = 8;       // duty 0 to 255
const float FAN_START_C = 25.0;
const float FAN_FULL_C  = 35.0;

DHT dht(DHT_PIN, DHT22);

void setup() {
  Serial.begin(115200);
  dht.begin();
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_BITS);
  Serial.println("E3 ready. Click the DHT22 and change the temperature.");
}

void loop() {
  float t = dht.readTemperature();
  if (isnan(t)) { Serial.println("DHT22 read failed"); delay(2000); return; }

  int duty;
  if (t <= FAN_START_C) duty = 0;
  else if (t >= FAN_FULL_C) duty = 255;
  else duty = (t - FAN_START_C) * 255 / (FAN_FULL_C - FAN_START_C);

  ledcWrite(FAN_PIN, duty);
  Serial.printf("temp=%.1f C  duty=%3d/255  fan=%3d %%\n", t, duty, duty * 100 / 255);
  delay(2000);                  // DHT22 can only be read every 2 s
}
