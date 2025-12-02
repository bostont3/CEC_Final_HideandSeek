// ============================= POT SETUP ===============================

#include <SPI.h>
#include <PCA95x5.h>
#include <Anitracks_ADS7142.h>
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

//Motor pins
#define AIN1 0
#define AIN2 1
#define BIN1 8
#define BIN2 A2

// ----------------------------
// Constants for motor
// ----------------------------
float R = 30.0; // Circle radius (cm)
float wheel_diameter = 6.5; // cm
float wheel_circ = 3.14159 * wheel_diameter;

float wheel_center_spacing = 12.5; // cm (midpoint of 10–15 cm)
float W2 = wheel_center_spacing / 2.0;

float R_L = R - W2; // Left wheel path radius
float R_R = R + W2; // Right wheel path radius

// Motor characteristics
int base_speed = 70; // 1 rev/sec for both wheels

const int LEFT_PWM  = 170;   // slower wheel
const int RIGHT_PWM = 255;   // faster wheel

unsigned long startTime;
int circle1=0;
// ============================= POT Defines ===============================

uint16_t temp0;
uint16_t pot;
float pot_360;
float lastpot=0, delta_pot;
char string[5];
char compare[] = "0123456789";
#define ADC_ADDR1 0x18   // ADS7142 I2C address
ADS7142 adc(ADC_ADDR1);  // Create ADS7142 object
PCA9535 muxU31;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Bluetooth define 
// Hider Wireless Peripheral
#include <ArduinoBLE.h>

//---------------------------
// UUIDs (remember to change)
//---------------------------
#define BLE_UUID_PERIPHERAL "ac11a091-6440-4762-ab94-ecb42db6c31c"
#define BLE_UUID_LED        "aebe45b9-92a3-45cc-8f55-2ed4c4ceccc2"
#define BLE_UUID_BUTTON     "eec126c7-0471-4d79-b821-e3c573770131"

#define HIDER_BUZZ_PIN 2  // Adjust for your Hider buzzer pin


//Bluetooth




// ============================= Matrix Display Setup ===============================

#include <PCA95x5.h>  // include library
PCA9535 LEDmux;
int number[] = { 0x3F0E, 0x060E, 0x5B0E, 0x4F0E, 0x660E, 0x6D0E, 0x7D0E, 0x070E, 0x7F0E, 0x670E };  // possibly unused
int i = 0, j = 0;                                                                                   // Index numbers changing in for loop
unsigned int num1[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67 };               // 0 to 9 on matrix display
unsigned int num2[] = { 0x07, 0x0B, 0x0D, 0x0E };                                                   // 1 to 4 display hex values
unsigned int printnum1, printnum2, printnum3, printnum4;                                            // where the two previous numbers are combined to a full matrix hex value


// ============================= POT Usage ===============================
float potget() {
  if (!adc.read2Ch(&temp0, &pot))  // could not figure out how to only use the temp channel shouldnt be a problem though
    Serial.println("adc failed");
  pot_360 = (((360 - 0) * (pot - 96)) / (65520 - 96)) + 0;  // scaling pot total to 0-360
  //Serial.print("POT = ");
  //Serial.println(pot_360);
  return pot_360;
}

// Move around circle
void circle(float delta_pot){
  for(int idx=0 ; idx<delta_pot; idx++){
    leftForward(LEFT_PWM);
    rightForward(RIGHT_PWM);
    delay(22.7);
  }
  stopMotors();
  
}
// Left wheel forward
void leftForward(int pwm) {
  analogWrite(AIN1, pwm);
  analogWrite(AIN2, 0);
}
 
// Right wheel forward
void rightForward(int pwm) {
  analogWrite(BIN1, 0);
  analogWrite(BIN2, pwm);
}
 

//stop motors
void stopMotors() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
  analogWrite(BIN1, 0);
  analogWrite(BIN2, 0);
}

  // ============================= SETUP ===============================
void setup() {

  // Serial
  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting...");
  hiderBLE_setup();
  startTime = millis(); // Record the start time

  // ============================= Matrix Display Initalization ===============================

  Wire.begin();               // this must be done before LEDmux
  LEDmux.attach(Wire, 0x21);  // 0x21 is the I2Caddress for the PCA9535 attached to the LEDs
  LEDmux.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  uint16_t mux_direction = 0x0010;  // mostly
  //outputs (0) //setting to 1 designates input
  LEDmux.direction(mux_direction);

  // ============================= POT initalization ===============================

  muxU31.attach(Wire, 0x20);
  muxU31.direction(0x1CFF);  // 1 is input, see schematic to get upper and lower bytes
  muxU31.write(PCA95x5::Port::P10, PCA95x5::Level::L);
  // disable SHT31
  delay(100);
  muxU31.write(PCA95x5::Port::P10, PCA95x5::Level::H);
  // enable SHT31
  delay(100);
  if (!sht31.begin(0x44)) {  // Set to 0x45 for alternate i2c addr
    Serial.println("Couldnt find SHT31");
    while (1)
      delay(1);
  }
    adc.begin();
}



  // ============================= LOOP ===============================
void loop() {

  pot_360 = potget();
  sprintf(string, "%f", pot_360);  // transfers 360 pot value to a string
  //Serial.println(string);
  //matrixidx(string, printnum1, printnum2, printnum3);
  // checking for under 100 value as matrix display moves above and below 100
  if (pot_360 >= 100) {
    for (j = 0; j <= 9; j++) {
      if (string[0] == compare[j]) {
        printnum1 = (num1[j] << 8) | num2[2];  // hundreds place
      }
      if (string[1] == compare[j]) {
        printnum2 = (num1[j] << 8) | num2[1];  // tens place
      }
      if (string[2] == compare[j]) {
        printnum3 = (num1[j] << 8) | num2[0];  // ones place
      }
    }
  } else if (pot_360<100 && pot_360>=10) {
    // pot<100
    printnum1 = (num1[0] << 8) | num2[2];  // hundreds set to zero
    for (j = 0; j <= 9; j++) {
      if (string[0] == compare[j]) {
        printnum2 = (num1[j] << 8) | num2[1];  // tens place
      }
      if (string[1] == compare[j]) {
        printnum3 = (num1[j] << 8) | num2[0];  // ones place
      }
    }
  }else if (pot_360<10){
    //under 10
    printnum1 = (num1[0] << 8) | num2[2];  // hundreds set to zero
    printnum2 = (num1[0] << 8) | num2[1];  // tens set to zero
    for (j = 0; j <= 9; j++) {
      if (string[0] == compare[j]) {
        printnum3 = (num1[j] << 8) | num2[0];  // ones place

  }
    }
  }


  // ============================= Matrix Display Usage ===============================
  //making the display work
  LEDmux.write(0x000F);
  LEDmux.write(printnum1);  // hundreds place

  LEDmux.write(0x000F);
  LEDmux.write(printnum2);  // tens place
  delay(0);
  LEDmux.write(0x000F);
  LEDmux.write(printnum3);  // ones place
  delay(0);

  if (millis() - startTime >= 2000 && circle1==0) {
    circle(pot_360);
    circle1++;
    Serial.println(pot_360);
  }
}
