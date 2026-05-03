#include <Arduino.h>
#include "esp_adc_cal.h"
#include "BQ25896.h"
#include <XPowersLib.h>
#define XPOWERS_CHIP_BQ25896
#define BQ27220_ADDR 0x55
#define I2C_SDA 18
#define I2C_SCL 17

float voltage = 0.0;
char voltage_String[10] = "";
BQ25896  bq(Wire);

void setupBattery(){
    Wire.begin();
    bq.begin();
    bq.setBatLoad(ENABLED);
    pinMode(2, OUTPUT); // Pin 2 controls power to the battery circuit
    digitalWrite(2, HIGH); // Turn on the power to the battery circuit
    delay(100); // Wait for the power to stabilize
}

float getBatteryVoltage(){
    return bq.getVBAT();
}

void shipMode(){
    bq.setBatLoad(DISABLED);
    digitalWrite(2, LOW); // Turn off the power to the battery circuit
    bq.setShipModeDelayed(); // Enter ship mode after a delay
}