#include <Arduino.h>
#include <Servo.h>

Servo myservo;
int serv_value = 0;
int raw_value;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  myservo.attach(9);
  myservo.write(0);

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Servo is at ");
  Serial.println(serv_value);
  if (Serial.available() > 0)
  {
    raw_value = Serial.readString().toInt();
    Serial.print("Got: ");
    Serial.println(raw_value);
    serv_value = raw_value % 181;
    //serv_value = map(raw_value, 0, 1023, 0, 180);
    myservo.write(serv_value);
    delay(15);
  }

}