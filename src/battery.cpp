#include <Arduino.h>
#include "esp_adc_cal.h"
#include <GaugeBQ27220.hpp>
#include <XPowersLib.h>
#define XPOWERS_CHIP_BQ25896
#define BQ27220_ADDR 0x55
#define I2C_SDA 3
#define I2C_SCL 2

char voltage_String[10] = "";
GaugeBQ27220 gauge;
PowersBQ25896 PPM;

uint16_t newDesignCapacity = 400;
uint16_t newFullChargeCapacity = 400;


// ============================
// Power Management
// ============================

void setupBattery(){
    if (!gauge.begin(Wire, I2C_SDA, I2C_SCL))
    {
        while (1)
        {
            Serial.println("Failed to BQ27220 - check your wiring!");

            delay(1000);
        }
    }
    Serial.println("Init BQ27220 Sensor success!");
    gauge.setNewCapacity(newDesignCapacity, newFullChargeCapacity);

    OperationConfig config = gauge.getOperationConfig();
}

float getBatteryVoltage(){
    if (gauge.refresh())
    {
        float voltage = gauge.getVoltage() / 1000.0; // Convert mV to V
        Serial.print("Battery Voltage: ");
        Serial.print(voltage);
        return voltage;
    }

    return 0.0f; // Return 0 if refresh fails
}

float getBatteryPercentage() {
    if (gauge.refresh()) {
        float percentage = ((float)gauge.getRemainingCapacity() / (float)newFullChargeCapacity) * 100.0f;

        Serial.print("Remaining Capacity: ");
        Serial.print(gauge.getRemainingCapacity());
        Serial.print(" (");
        Serial.print(percentage, 1);
        Serial.println("%)");

        return percentage;
    }

    return 0.0f;
}

void chargerInit()
{
  bool result = PPM.init(Wire, I2C_SDA, I2C_SCL, BQ25896_SLAVE_ADDRESS);
  if (result == false)
  {
    Serial.println("PPM is not online...");
  }
  else
  {
    Serial.println("Init PPM success!");
    PPM.setSysPowerDownVoltage(3200);
    PPM.setChargeTargetVoltage(4208); // 3364mv
    PPM.setPrechargeCurr(64);
    PPM.setChargerConstantCurr(64);

    if (PPM.getVbusVoltage() > 2800)
    {
      PPM.enableCharge();
      Serial.println("USB connected, charging enabled.");
    }
    else
    {
      PPM.disableCharge();
      Serial.println("USB not connected, charging disabled.");
    }
  }
}