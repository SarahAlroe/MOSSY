#define CONTROL_PIN A4

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(CONTROL_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    byte data = Serial.read();
    if (data == 114){
      digitalWrite(CONTROL_PIN, HIGH);
      digitalWrite(LED_BUILTIN, HIGH);
    }else{
      digitalWrite(CONTROL_PIN, LOW);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
