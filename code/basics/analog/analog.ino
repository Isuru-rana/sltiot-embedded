void setup() {

  Serial.begin(115200);
  pinMode(18, OUTPUT);

}

void loop() {

  int pot;
  pot = analogRead(34);

  int led;
  led = pot * 255 / 4095;

  analogWrite(18, led);

  Serial.print("pot:  ");
  Serial.print(pot);

  Serial.print("  led:  ");
  Serial.println(led);

}
