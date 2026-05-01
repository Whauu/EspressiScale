#include "HX711Driver.h"
#include <Arduino.h>
#include <math.h>

HX711Driver::HX711Driver(int doutPin, int sckPin)
    : doutPin(doutPin), sckPin(sckPin) {}

void HX711Driver::begin() {
    hx.begin(doutPin, sckPin);
    hx.set_scale(calibrationFactor);
    hx.tare();
}

bool HX711Driver::isReady() {
    return hx.is_ready();
}

bool HX711Driver::tare() {
    int times = 20;
	long sum = 0;
    long lastSum = 0;
    bool finished = false;
    int stableCounter = 0;

	for (byte i = 0; i < times && !finished; i++) {
		sum = hx.read();
    if (sum == lastSum) {
      stableCounter++;
      if (stableCounter >= 2) {
        finished = true;
      }
    }
    else {
      stableCounter = 0;
    }
    lastSum = sum;
    hx.set_offset(sum); // Set the scale to 0.0
		delay(0);
	}
  finished = true;
  return finished;
}

float HX711Driver::read() {
    if (!hx.is_ready()) return NAN;
    return hx.get_units();
}

bool HX711Driver::calibrate(float referenceWeight) {
    bool finished = false;
    float calibration_factors[5];
    float measured_values[5];
    if (referenceWeight <= 0) {
      referenceWeight = 100; // Default to 100g if invalid
    }

    for (int trial = 0; trial < 5; ++trial) {
      // Reset calibration factor to 1 before each trial
      calibrationFactor = 1;
      hx.set_scale(calibrationFactor);

      float measured = hx.get_units(2);
      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(": measured value = ");
      Serial.println(measured, 4);

      // Calibration loop: adjust calibration_factor so measured == 100
      Serial.println("Calibrating...");
      float tolerance = 0.05;
      int max_iterations = 1000;
      int iter = 0;
      float trial_calibration_factor = calibrationFactor;

      while (abs(measured - referenceWeight) > tolerance && iter < max_iterations) {
        trial_calibration_factor *= (measured / referenceWeight); // Adjust factor proportionally
        hx.set_scale(trial_calibration_factor);
        measured = hx.get_units(2);
        iter++;
      }

      calibration_factors[trial] = trial_calibration_factor;
      measured_values[trial] = measured;

      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(" calibration factor: ");
      Serial.println(trial_calibration_factor, 6);
      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(" measured value: ");
      Serial.println(measured, 4);

      delay(1000); // Short delay between trials
    }

    // Calculate average calibration factor and measured value
    float sum_factor = 0;
    float sum_measured = 0;
    for (int i = 0; i < 5; ++i) {
      sum_factor += calibration_factors[i];
      sum_measured += measured_values[i];
    }
    float avg_factor = sum_factor / 5.0;
    float avg_measured = sum_measured / 5.0;

    Serial.println("Calibration complete.");
    Serial.print("Your calibration factor: ");
    Serial.println(avg_factor, 4);

    calibrationFactor = avg_factor;
    EEPROM.put(0, calibrationFactor);
    EEPROM.commit();
    hx.set_scale(calibrationFactor);
    finished = true;
    return finished;
}

float HX711Driver::getFactor() const {
    return calibrationFactor;
}

void HX711Driver::setFactor(float newOffset) {
    calibrationFactor = newOffset;
    hx.set_scale(calibrationFactor);
}

void HX711Driver::setOffset(float newOffset) {
    offset = newOffset;
    hx.set_offset(offset);
}

float HX711Driver::getOffset() {
    return hx.get_offset();
}