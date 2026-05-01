#include <Arduino.h>
#include "esp_adc_cal.h"
#define BAT_ADC    2

float voltage = 0.0;
char voltage_String[10] = "";
uint32_t readADC_Cal(int ADC_Raw);

void setupBattery(){
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
}

float getBatteryVoltage(){
    voltage = (readADC_Cal(analogRead(BAT_ADC)));
    voltage = voltage * 0.002; // we use a voltage divider 1:1
    return voltage;
}

uint32_t readADC_Cal(int ADC_Raw)
{
    esp_adc_cal_characteristics_t adc_chars;

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
