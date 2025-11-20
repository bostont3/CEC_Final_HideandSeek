#include <Wire.h>       // I2C library
#include <VL6180X.h>    // distance sensor library
#include <PCA95x5.h>    // I2C to GPIO library
#include <Servo.h>      // servo library
 
// Valid scaling factors are 1, 2, or 3.
#define SCALING 3
 
// Motor pins 
#define AIN1 0
#define AIN2 1
#define BIN1 8
#define BIN2 A2
#define BUZZ_PIN 2  // buzzer
#define NEO_PIN   17 //led setup
#define NEO_COUNT 18  
VL6180X sensor;        
PCA9535 muxU31;         
 
// ============================= SERVO ===============================
 
#define SERVO_PIN 9
 
const int SERVO_MIN_ANGLE = 40;
const int SERVO_MAX_ANGLE = 140;
const int SERVO_STEP      = 10;    // step size during sweep
const int SERVO_CENTER    = 90;    // center the servo motor
 
Servo myservo;
 
// ======================== BEHAVIOR CONSTANTS ========================
 
const uint16_t DETECTION_THRESHOLD_MM = 500;  // detect hider within 500 mm
const uint16_t STOP_DISTANCE_MM       = 80;   
 
const unsigned long SERVO_SETTLE_MS   = 100;  
const unsigned long TURN_120_MS       = 800;  

const unsigned long DRIVE_50CM_MS     = 1500;
 
bool hiderFound = false;
 
// ========================= MOTOR FUNCTIONS ==========================
 
void move_forward() {
  analogWrite(AIN1, 200);
  analogWrite(AIN2, 0);
  analogWrite(BIN1, 0);
  analogWrite(BIN2, 200);
}
 
void stop_motors() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
  analogWrite(BIN1, 0);
  analogWrite(BIN2, 0);
}
 
void turn_left() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 200);
  analogWrite(BIN1, 0);    
  analogWrite(BIN2, 200);
}

// ======================= NEOPIXEL ==============================
 Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
 
// Helper: set all pixels to an RGB color
void setAllPixels(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NEO_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}
// ======================= SENSOR RANGE ==============================
 
uint16_t readDistanceSafe() {
  uint16_t dist = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred()) {
    return 0;
  }
  return dist;
}
 
// ======================= ACTION FUNCTIONS =========================
 
// Sweep the servo across 120° and look for a target within 500 mm.
bool sweepForHider() {
  for (int angle = SERVO_MIN_ANGLE; angle <= SERVO_MAX_ANGLE; angle += SERVO_STEP) {
    myservo.write(angle);
    delay(100);
 
    uint16_t dist = readDistanceSafe();
    if (dist > 0 && dist <= DETECTION_THRESHOLD_MM) {
      return true;
    }
  }
 
  myservo.write(SERVO_CENTER);
  return false;
}
 
void rotateBaseLeft() {
  turn_left();
  delay(TURN_120_MS);
  stop_motors();
}
 
void driveToHider() {
  unsigned long startTime = millis();
  while (millis() - startTime < DRIVE_50CM_MS) {
    uint16_t dist = readDistanceSafe();
 
   
    if (dist > 0 && dist <= STOP_DISTANCE_MM) {
      break;
    }
 
    move_forward();
  }
  stop_motors();
}
 
void policeSirenSweep() {
  // Sweep up
  for (int f = 600; f <= 1200; f += 20) {
    tone(BUZZ_PIN, f);
    delay(8);
  }
  // Sweep down
  for (int f = 1200; f >= 600; f -= 20) {
    tone(BUZZ_PIN, f);
    delay(8);
  }
  noTone(BUZZ_PIN);
}
 
void policeMode() {
  while (true) {
    // Blue 
    setAllPixels(0, 0, 255);
    policeSirenSweep();
 
    // Red 
    setAllPixels(255, 0, 0);
    policeSirenSweep();
 
    setAllPixels(255, 255, 255);
    delay(200);
    setAllPixels(0, 0, 0);
    delay(200);
  }
} 
 // ============================== SETUP ===============================

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Seeker starting...");
 
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
 
  Wire.begin();
 
  // I2C to GPIO chip setup 
  muxU31.attach(Wire, 0x20);
  muxU31.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  muxU31.direction(0x1CFF);
  muxU31.write(PCA95x5::Port::P11, PCA95x5::Level::H);  // enable VL6180 distance sensor
 
  // Distance sensor setup
  sensor.init();
  sensor.configureDefault();
  sensor.setScaling(SCALING);
  sensor.setTimeout(100);
 
  // Servo setup
  myservo.attach(SERVO_PIN);
  myservo.write(SERVO_CENTER);

  // NeoPixel setup
  strip.begin();
  strip.show();     
  strip.setBrightness(255);
}

 // =============================== LOOP ===============================

void loop() {
  if (hiderFound) {
    stop_motors();
    policeMode();
    return;
  }
 
  // Perform up to 3 sweeps of 120°.
  bool found = false;
  for (int sweepIndex = 0; sweepIndex < 3; sweepIndex++) {
 
    //check for hider
    if (sweepForHider()) {
      Serial.println("Hider detected!");
      found = true;
      break;
    }
 
    if (sweepIndex < 2) {
      rotateBaseLeft();
    }
  }
 
  if (found) {
    driveToHider();
    hiderFound = true;   
  } else {
    stop_motors();
    delay(500);
  }
 
  delay(100);
}