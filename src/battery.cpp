#include <Arduino.h>
#include "esp_adc_cal.h"
#include <GaugeBQ27220.hpp>
#ifndef SENSOR_SDA
#define SENSOR_SDA 3
#endif

#ifndef SENSOR_SCL
#define SENSOR_SCL 2
#endif

char voltage_String[10] = "";
GaugeBQ27220 gauge;

uint16_t newDesignCapacity = 3500;
uint16_t newFullChargeCapacity = 3500;

void setupBattery(){
    if (!gauge.begin(Wire, SENSOR_SDA, SENSOR_SCL))
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
