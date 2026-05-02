#pragma once

#include "IScaleDriver.h"
#include <ADS1232.h>
#include <EEPROM.h>

class ADS1232Driver : public IScaleDriver {
public:
    ADS1232Driver(int doutPin, int sckPin, int pdwnPin, int speedPin);

    void begin() override;
    bool isReady() override;

    bool tare() override;
    float read() override;

    bool calibrate(float referenceWeight) override;

    float getFactor() const override;
    void setFactor(float offset) override;
    float getOffset() override;
    void setOffset(float offset) override;

private:
    ADS1232 adc;

    int doutPin;
    int sckPin;
    int pdwnPin;
    int speedPin;

    float offset = 0.0f;
    float calibrationFactor = 1.0f;
};