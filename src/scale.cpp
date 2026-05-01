#include "scale.h"
#include "IScaleDriver.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <math.h>

#if defined(SCALE_USE_HX711)
#include "HX711Driver.h"
#elif defined(SCALE_USE_ADS1232)
#include "ADS1232Driver.h"
#else
#error "No scale driver selected. Define SCALE_USE_HX711 or SCALE_USE_ADS1232 in platformio.ini"
#endif

static constexpr int EEPROM_SIZE = 512;
static constexpr int EEPROM_ADDR_OFFSET = 0;

// HX711 pins
static constexpr int HX711_DOUT_PIN = 4;
static constexpr int HX711_SCK_PIN = 5;

// ADS1232 pins
static constexpr int ADS1232_DOUT_PIN = 5;
static constexpr int ADS1232_SCK_PIN = 7;
static constexpr int ADS1232_PDWN_PIN = 8;
static constexpr int ADS1232_SPEED_PIN = 4;

static IScaleDriver* scaleDriver = nullptr;

static void loadScaleSettings() {
    float storedFactor = 0.0f;
    EEPROM.get(EEPROM_ADDR_OFFSET, storedFactor);

    if (!isnan(storedFactor)) {
        scaleDriver->setFactor(storedFactor);
    }
}

static void saveScaleSettings() {
    EEPROM.put(EEPROM_ADDR_OFFSET, scaleDriver->getFactor());
    EEPROM.commit();
}

void setupScale() {
    EEPROM.begin(EEPROM_SIZE);

#if defined(SCALE_USE_HX711)
    scaleDriver = new HX711Driver(HX711_DOUT_PIN, HX711_SCK_PIN);
#elif defined(SCALE_USE_ADS1232)
    scaleDriver = new ADS1232Driver(
        ADS1232_DOUT_PIN,
        ADS1232_SCK_PIN,
        ADS1232_PDWN_PIN,
        ADS1232_SPEED_PIN
    );
#endif

    scaleDriver->begin();
    loadScaleSettings();
    tareScale();
}

bool tareScale() {
    if (!scaleDriver) return false;

    bool ok = scaleDriver->tare();

    if (ok) {
        saveScaleSettings();
    }

    return ok;
}

float updateScale() {
    if (!scaleDriver) return NAN;
    return scaleDriver->read();
}

bool doCalibration(float referenceWeight) {
    if (!scaleDriver) return false;
    return scaleDriver->calibrate(referenceWeight);
}

float getFactor() {
    if (!scaleDriver) return 0.0f;
    return scaleDriver->getFactor();
}

bool setFactor(float newFactor) {
    if (!scaleDriver) return false;

    scaleDriver->setFactor(newFactor);
    saveScaleSettings();

    return true;
}

bool setOffset(float newOffset) {
    if (!scaleDriver) return false;

    scaleDriver->setOffset(newOffset);

    return true;
}

float getOffset() {
    if (!scaleDriver) return 0.0f;
    return scaleDriver->getOffset();
}