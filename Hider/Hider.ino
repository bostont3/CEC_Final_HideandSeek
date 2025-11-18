#define AIN1 0
#define AIN2 1

void setup() {
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
}

void loop() {
  if(dist<120){

  analogWrite(AIN1, 75);
  analogWrite(AIN2, 0);
  analogWrite(BIN1,75);
  analogWrite(BIN2,75);
