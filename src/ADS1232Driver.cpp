#include "ADS1232Driver.h"
#include <Arduino.h>
#include <math.h>

ADS1232Driver::ADS1232Driver(int doutPin, int sckPin, int pdwnPin, int speedPin)
    : doutPin(doutPin),
      sckPin(sckPin),
      pdwnPin(pdwnPin),
      speedPin(speedPin) {}

void ADS1232Driver::begin() {
    adc.begin(doutPin, sckPin, pdwnPin, speedPin);
    pinMode(pdwnPin, OUTPUT);
    digitalWrite(pdwnPin, HIGH);
}

bool ADS1232Driver::isReady() {
    return true;
}

bool ADS1232Driver::tare() {
    int times = 20;
	long sum = 0;
    long lastSum = 0;
    bool finished = false;
    int stableCounter = 0;

	for (byte i = 0; i < times && !finished; i++) {
		adc.read(sum);
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
    adc.set_offset(sum); // Set the scale to 0.0
		delay(0);
	}
  finished = true;
  return finished;
}

float ADS1232Driver::read() {
    float updated = 0.0f;
    adc.get_units(updated, 1);
    return updated;
}

bool ADS1232Driver::calibrate(float referenceWeight) {
    bool finished = false;
    float calibration_factors[5];
    float measured_values[5];
    if (referenceWeight <= 0) {
      referenceWeight = 100; // Default to 100g if invalid
    }

    for (int trial = 0; trial < 5; ++trial) {
      // Reset calibration factor to 1 before each trial
      calibrationFactor = 1;
      adc.set_scale(calibrationFactor);

      float measured;
      adc.get_units(measured, 2);
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
        adc.set_scale(trial_calibration_factor);
        adc.get_units(measured, 2);
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
    adc.set_scale(calibrationFactor);
    finished = true;
    return finished;
}

float ADS1232Driver::getFactor() const {
    return calibrationFactor;
}

void ADS1232Driver::setFactor(float newFactor) {
    adc.set_scale(newFactor);
    calibrationFactor = newFactor;
}

float ADS1232Driver::getOffset() {
  return adc.get_offset();
}