// ============================= POT SETUP ===============================

#include <SPI.h>
#include <PCA95x5.h>
#include <Anitracks_ADS7142.h>
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

// ============================= POT Defines ===============================

uint16_t temp0;
uint16_t pot;
float pot_360;
char string[5];
char compare[] = "0123456789";
#define ADC_ADDR1 0x18   // ADS7142 I2C address
ADS7142 adc(ADC_ADDR1);  // Create ADS7142 object
PCA9535 muxU31;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

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
  Serial.print("POT = ");
  Serial.println(pot_360);
  return pot_360;
}


// ============================= Matrix Display Changing ===============================
void matrixidx(char *string, uint16_t &printnum1, uint16_t &printnum2, uint16_t &printnum3) {

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
  } else {
    // pot<100
    printnum3 = (num1[0] << 8) | num2[0];  // hundreds set to zero
    for (j = 0; j <= 9; j++) {
      if (string[1] == compare[j]) {
        printnum2 = (num1[j] << 8) | num2[1];  // tens place
      }
      if (string[2] == compare[j]) {
        printnum3 = (num1[j] << 8) | num2[0];  // ones place
      }
    }
  }
}


  // ============================= SETUP ===============================
void setup() {

  // Serial
  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting...");


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
  Serial.println(string);
  matrixidx(string, printnum1, printnum2, printnum3);


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
}
