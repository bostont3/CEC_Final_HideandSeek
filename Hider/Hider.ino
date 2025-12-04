#include <SPI.h>
#include <PCA95x5.h>
#include <Anitracks_ADS7142.h>
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

// ----------------- MOTOR PINS -----------------
#define AIN1 0
#define AIN2 1
#define BIN1 8
#define BIN2 A2

#define LEFT_PWM 170
#define RIGHT_PWM 255

// ----------------- ADS7142 -----------------
ADS7142 adc(0x18);
uint16_t temp0, pot;
float pot_360;

// ----------------- MATRIX DISPLAY -----------
PCA9535 LEDmux;
char strbuf[5];

unsigned int num1[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x67};
unsigned int num2[] = {0x07,0x0B,0x0D,0x0E};
unsigned int d1,d2,d3;

// ----------------- NEOPIXEL -----------------
#define NEO_PIN 17
#define NEO_COUNT 18
Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

// ----------------- BUZZER --------------------
#define BUZZ_PIN 2

// ----------------- BLE -----------------------
#define BLE_UUID_TEST_SERVICE  "ac11a091-6440-4762-ab94-ecb42db6c31c"
#define BLE_UUID_SENSOR1_VALUE "aebe45b9-92a3-45cc-8f55-2ed4c4ceccc2"

// ----------------- FUNCTIONS -----------------
float readPot() {
    adc.read2Ch(&temp0,&pot);
    float v = (360.0*(pot-96))/(65520-96);
    if (v < 0) v = 0;
    if (v > 360) v = 360;
    return v;
}

void updateDisplay(int angle) {
    sprintf(strbuf,"%03d",angle);

    d1 = (num1[strbuf[0]-'0'] << 8) | num2[2];
    d2 = (num1[strbuf[1]-'0'] << 8) | num2[1];
    d3 = (num1[strbuf[2]-'0'] << 8) | num2[0];

    LEDmux.write(0x000F); LEDmux.write(d1);
    LEDmux.write(0x000F); LEDmux.write(d2);
    LEDmux.write(0x000F); LEDmux.write(d3);
}

void leftForward(int p){ analogWrite(AIN1,p); analogWrite(AIN2,0); }
void rightForward(int p){ analogWrite(BIN1,0); analogWrite(BIN2,p); }
void stopMotors(){ analogWrite(AIN1,0); analogWrite(AIN2,0); analogWrite(BIN1,0); analogWrite(BIN2,0); }

void rotateTo(float ang) {
    for (int i=0;i<(int)ang;i++){
        leftForward(LEFT_PWM);
        rightForward(RIGHT_PWM);
        delay(23);
    }
    stopMotors();
}

// ----------------- SETUP ---------------------
bool moved = false;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Wire.begin();
    LEDmux.attach(Wire,0x21);
    LEDmux.direction(0x0010);

    adc.begin();

    strip.begin();
    strip.show();

    pinMode(BUZZ_PIN,OUTPUT);

    BLE.begin();
    BLE.setLocalName("HIDER");

    Serial.println("WAITING 5 SECONDS FOR BEARING INPUT...");
    delay(5000);  // *** NEW WAIT PERIOD ***
}

// ----------------- LOOP -----------------------
void loop() {

    pot_360 = readPot();
    updateDisplay((int)pot_360);

    // move once
    if (!moved) {
        rotateTo(pot_360);
        moved = true;
        BLE.scanForUuid(BLE_UUID_TEST_SERVICE);
        Serial.println("Movement done, scanning for seeker...");
        return;
    }

    // ------------------ BLE CONNECTION ----------------------
    BLEDevice dev = BLE.available();
    if (!dev) return;

    Serial.println("Device detected");
    BLE.stopScan();

    if (!dev.connect()) {
        Serial.println("FAILED connect()");
        BLE.scanForUuid(BLE_UUID_TEST_SERVICE);
        return;
    }
    Serial.println("Seeker connected — Hider FOUND");

    // *** SOLID YELLOW + BUZZER SIGNAL ***
    strip.fill(strip.Color(255,255,0));
    strip.show();
    tone(BUZZ_PIN, 1200, 800);  // loud, clear buzz

    // keep connection alive but no flashing logic
    BLECharacteristic c = dev.characteristic(BLE_UUID_SENSOR1_VALUE);

    while (dev.connected()) {
        delay(200);
    }

    dev.disconnect();
    BLE.scanForUuid(BLE_UUID_TEST_SERVICE);
}
