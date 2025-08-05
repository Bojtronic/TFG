int pin_pulsador=4;
int pin_led=2;

void setup() {
  pinMode(pin_pulsador, INPUT);
  pinMode(pin_led, OUTPUT);
}

void loop() {
  if(digitalRead(pin_pulsador) == HIGH){
    digitalWrite(pin_led, HIGH);
  }
  else{
    digitalWrite(pin_led, LOW);
  }
  delay(10);
}
