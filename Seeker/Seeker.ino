#include <Servo.h>

Servo myservo;
int pos_servo = 0;


void setup() {
  Serial.begin(115200);
  delay(1000);
  //SERVO
  myservo.attach(9);
}

void loop() {
  if (pos_servo < 180) {
    myservo.write(pos_servo);
    delay(100);
    pos_servo += 10;
  } else {
    myservo.write(0);
    pos_servo=0;
  }
}
