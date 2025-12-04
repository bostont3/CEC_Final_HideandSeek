#include <Wire.h>
#include <VL6180X.h>
#include <PCA95x5.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoBLE.h>

// ------------------------------------------------------------
#define SCALING 3
#define DETECT_MM 500
#define STOP_MM   80

#define AIN1 0
#define AIN2 1
#define BIN1 8
#define BIN2 A2

#define SERVO_PIN 9
#define BUZZ_PIN  2

#define NEO_PIN   17
#define NEO_COUNT 18

// BLE UUIDs
#define BLE_UUID_TEST_SERVICE  "ac11a091-6440-4762-ab94-ecb42db6c31c"
#define BLE_UUID_SENSOR1_VALUE "aebe45b9-92a3-45cc-8f55-2ed4c4ceccc2"

// ------------------------------------------------------------
VL6180X sensor;
PCA9535 muxU31;
Servo myservo;
Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

BLEService testService(BLE_UUID_TEST_SERVICE);
BLEUnsignedCharCharacteristic Sensor1Characteristic(
  BLE_UUID_SENSOR1_VALUE, BLENotify);

// ------------------------------------------------------------
void forward() {
  analogWrite(AIN1, 200);
  analogWrite(AIN2, 0);
  analogWrite(BIN1, 0);
  analogWrite(BIN2, 200);
}

void stopMotors() {
  analogWrite(AIN1,0);
  analogWrite(AIN2,0);
  analogWrite(BIN1,0);
  analogWrite(BIN2,0);
}

void rotateLeft120() {
  analogWrite(AIN1,0);
  analogWrite(AIN2,200);
  analogWrite(BIN1,0);
  analogWrite(BIN2,200);
  delay(800);
  stopMotors();
}

uint16_t readDist() {
  uint16_t d = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred()) return 9999;
  return d;
}

bool sweepDetect() {
  for (int a = 40; a <= 140; a += 10) {
    myservo.write(a);
    delay(120);
    if (readDist() < DETECT_MM) return true;
  }
  myservo.write(90);
  return false;
}

void driveToHider() {
  unsigned long t0 = millis();
  while (millis() - t0 < 2000) {
    if (readDist() < STOP_MM) break;
    forward();
  }
  stopMotors();
}

void policeMode() {
  while (true) {
    strip.fill(strip.Color(0,0,255));
    strip.show();
    tone(BUZZ_PIN, 900);
    delay(300);

    strip.fill(strip.Color(255,0,0));
    strip.show();
    tone(BUZZ_PIN, 1200);
    delay(300);
  }
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  // motors
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  pinMode(BIN1,OUTPUT);
  pinMode(BIN2,OUTPUT);

  // sensor enable
  Wire.begin();
  muxU31.attach(Wire,0x20);
  muxU31.direction(0x1CFF);
  muxU31.write(PCA95x5::Port::P11,PCA95x5::Level::H);

  sensor.init();
  sensor.configureDefault();
  sensor.setScaling(SCALING);
  sensor.setTimeout(100);

  myservo.attach(SERVO_PIN);
  myservo.write(90);

  strip.begin();
  strip.show();

  // BLE
  BLE.begin();
  BLE.setLocalName("SEEKER");
  BLE.setAdvertisedService(testService);
  testService.addCharacteristic(Sensor1Characteristic);
  BLE.addService(testService);
  BLE.advertise();
}

// ------------------------------------------------------------
void loop() {

  bool detected = false;

  // 3 sweeps of 120 degrees
  for (int k=0; k<3; k++) {
    if (sweepDetect()) { detected = true; break; }
    if (k<2) rotateLeft120();
  }

  BLEDevice central = BLE.central();

  if (central && central.connected()) {
    static unsigned long lastTx = 0;
    if (detected && millis() - lastTx > 300) {
      Sensor1Characteristic.writeValue((uint8_t)1);
      lastTx = millis();
    }
  }

  if (detected) {
    driveToHider();
    policeMode();
  }

  delay(100);
}
