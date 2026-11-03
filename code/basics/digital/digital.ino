void setup() {

  pinMode(18, OUTPUT);
  pinMode(14, INPUT);


}

void loop() {

  int switch1;

  switch1 = digitalRead(14);

  if(switch1 == 1){
    digitalWrite(18, HIGH);
  }
  else {
    digitalWrite(18, LOW);
  }

}
