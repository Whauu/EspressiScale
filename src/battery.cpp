#include <Arduino.h>
#include "esp_adc_cal.h"
#include "BQ25896.h"
#include <XPowersLib.h>
#define XPOWERS_CHIP_BQ25896
#define BQ27220_ADDR 0x55
#define I2C_SDA 18
#define I2C_SCL 17
#define OTG_PIN 2

float voltage = 0.0;
char voltage_String[10] = "";
BQ25896  bq(Wire);

void setupBattery(){
    Wire.begin();
    bq.begin();
    bq.setBatLoad(ENABLED);
    pinMode(OTG_PIN, OUTPUT); // Pin 2 controls power to the battery circuit
    digitalWrite(OTG_PIN, HIGH); // Turn on the power to the battery circuit
    delay(100); // Wait for OTG pin set high to stabilize
    bq.setOTGEnable(ENABLED); // Enable OTG power for the battery circuit
    delay(100); // Wait for the battery circuit to stabilize
    printBQStatus();
}

float getBatteryVoltage(){
    return bq.getVBAT();
}

void shipMode(){
    bq.setBatLoad(DISABLED);
    digitalWrite(OTG_PIN, LOW); // Turn off the power to the battery circuit
    bq.setOTGEnable(DISABLED); // Disable OTG power to ensure battery is fully powered down
    bq.setShipModeDelayed(); // Enter ship mode after a delay
}

void printBQStatus()
{
    bq.properties();

    Serial.print("VBUS: ");
    Serial.print(bq.getVBUS());
    Serial.println(" V");

    Serial.print("VSYS: ");
    Serial.print(bq.getVSYS());
    Serial.println(" V");

    Serial.print("VBAT: ");
    Serial.print(bq.getVBAT());
    Serial.println(" V");

    Serial.print("VBUS_STATUS: ");

    switch (bq.getVBUS_STATUS())
    {
        case EmbeddedDevices::BQ25896<1>::VBUS_STAT::NO_INPUT:
            Serial.println("NO_INPUT");
            break;

        case EmbeddedDevices::BQ25896<1>::VBUS_STAT::USB_HOST:
            Serial.println("USB_HOST");
            break;

        case EmbeddedDevices::BQ25896<1>::VBUS_STAT::ADAPTER:
            Serial.println("ADAPTER");
            break;

        case EmbeddedDevices::BQ25896<1>::VBUS_STAT::OTG:
            Serial.println("OTG");
            break;

        default:
            Serial.println("UNKNOWN");
            break;
    }
}