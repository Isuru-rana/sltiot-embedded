// E2 ANALOG IN (ADC): how wet is the soil?
// The ESP32 ADC turns 0 to 3.3 V into a number from 0 to 4095 (12 bits).
// In Wokwi the SOIL knob stands in for a soil moisture probe.

const int SOIL_PIN = 34;      // input-only pin, ADC1
const int SAMPLES  = 16;      // average to remove noise
const int DRY_BELOW = 30;     // %

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("E2 ready. Turn the SOIL knob.");
}

void loop() {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) sum += analogRead(SOIL_PIN);
  int raw = sum / SAMPLES;

  float volts = raw * 3.3 / 4095.0;
  int moisture = map(raw, 0, 4095, 0, 100);

  Serial.printf("raw=%4d  volts=%.2f V  moisture=%3d %%  %s\n",
                raw, volts, moisture, moisture < DRY_BELOW ? "DRY: needs water" : "ok");
  delay(1000);
}
