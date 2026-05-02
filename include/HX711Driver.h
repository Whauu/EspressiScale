#pragma once

#include "IScaleDriver.h"
#include <HX711.h>
#include <EEPROM.h>

class HX711Driver : public IScaleDriver {
public:
    HX711Driver(int doutPin, int sckPin);

    void begin() override;
    bool isReady() override;

    bool tare() override;
    float read() override;

    bool calibrate(float knownWeight) override;

    float getFactor() const override;
    void setFactor(float offset) override;
    void setOffset(float offset) override;
    float getOffset() override;

private:
    HX711 hx;
    int doutPin;
    int sckPin;

    float calibrationFactor = 1.0f;
    float offset = 0.0f;
};