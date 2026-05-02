#pragma once

class IScaleDriver {
public:
    virtual void begin() = 0;
    virtual bool isReady() = 0;

    virtual bool tare() = 0;
    virtual float read() = 0;

    virtual bool calibrate(float knownWeight) = 0;

    virtual float getFactor() const = 0;
    virtual void setFactor(float offset) = 0;
    virtual float getOffset() = 0;
    virtual void setOffset(float offset) = 0;

    virtual ~IScaleDriver() = default;
};