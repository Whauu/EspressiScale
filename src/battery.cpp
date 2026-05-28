#include <Arduino.h>
#include "esp_adc_cal.h"
#include "BQ25896.h"
#include <XPowersLib.h>
#define XPOWERS_CHIP_BQ25896
#define I2C_SDA 18
#define I2C_SCL 17
#define OTG_PIN 2
#define BQ_ADDR 0x6A

float voltage = 0.0;
char voltage_String[10] = "";
BQ25896  bq(Wire);

void setupBattery(){
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    Serial.println("Reading BQ registers...");

    uint8_t reg03 = readBQ(0x03); // SYS_CTRL
    uint8_t reg0B = readBQ(0x0B); // VBUS_STAT
    uint8_t reg0C = readBQ(0x0C); // FAULT
    uint8_t reg0E = readBQ(0x0E); // BATV
    uint8_t reg14 = readBQ(0x14); // CTRL2 

    Serial.print("REG03 SYS_CTRL = 0x"); Serial.println(reg03, HEX);
    Serial.print("REG0B STATUS   = 0x"); Serial.println(reg0B, HEX);
    Serial.print("REG0C FAULT    = 0x"); Serial.println(reg0C, HEX);
    Serial.print("REG0E BATV     = 0x"); Serial.println(reg0E, HEX);
    Serial.print("REG14 CTRL2    = 0x"); Serial.println(reg14, HEX);
    /*Wire.begin();
    bq.begin();
    bq.setBatLoad(ENABLED);
    pinMode(OTG_PIN, OUTPUT); // Pin 2 controls power to the battery circuit
    digitalWrite(OTG_PIN, HIGH); // Turn on the power to the battery circuit
    delay(100); // Wait for OTG pin set high to stabilize
    bq.setOTGEnable(ENABLED); // Enable OTG power for the battery circuit
    delay(100); // Wait for the battery circuit to stabilize
    printBQStatus();*/
}

float getBatteryVoltage(){
    bq.properties();
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
uint8_t readBQ(uint8_t reg)
{
    Wire.beginTransmission(BQ_ADDR);
    Wire.write(reg);

    uint8_t err = Wire.endTransmission(false); // repeated start
    if (err != 0) {
        Serial.print("Write reg failed, err=");
        Serial.println(err);
        return 0xFF;
    }

    uint8_t n = Wire.requestFrom(BQ_ADDR, (uint8_t)1, (uint8_t)true);
    if (n != 1) {
        Serial.println("Read failed");
        return 0xFF;
    }

    return Wire.read();
}