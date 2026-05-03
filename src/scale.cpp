#include <Arduino.h>
#include <EEPROM.h>
#include <ADS1232.h>

#define LOADCELL_DOUT_PIN  5
#define LOADCELL_SCK_PIN   7
#define LOADCELL_POWER_PIN 8
#define LOADCELL_SPEED     4
#define ADC_POWER_EN             6

float calibration_factor = 1.0f; // Default value
const int CALIBRATION_FACTOR_ADDR = 0; // EEPROM address
bool isCalibrating = false;

ADS1232 scale;

void setupScale(){
  EEPROM.begin(512);
  EEPROM.get(CALIBRATION_FACTOR_ADDR, calibration_factor);
  if (isnan(calibration_factor) || calibration_factor == 0) {
    calibration_factor = 1; // Reset to default if invalid
  }
  pinMode(ADC_POWER_EN, OUTPUT);
  digitalWrite(ADC_POWER_EN, HIGH); // Power on the 4.5V LDO for the ADC
  delay(100); // Wait for the ADC to stabilize
  pinMode(LOADCELL_POWER_PIN, OUTPUT);
  digitalWrite(LOADCELL_POWER_PIN, HIGH);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, LOADCELL_POWER_PIN, LOADCELL_SPEED, FAST);
  scale.set_scale(calibration_factor);
  scale.tare(); 
}

float getOffset(){
  return scale.get_offset();
}

bool setOffset(float newOffset){
  scale.set_offset(newOffset);
  return true;
}

bool tareScale(){
  //delay(500);
  int times = 20;
	long sum = 0;
  long lastSum = 0;
  bool finished = false;
  int stableCounter = 0;

	for (byte i = 0; i < times && !finished; i++) {
		scale.read(sum);
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
    scale.set_offset(sum); // Set the scale to 0.0
		delay(0);
	}
  finished = true;
  return finished;
}

float updateScale(){
  float updated = 0.0f;
  scale.get_units(updated, 1);
  return updated;
}

bool doCalibration(int referenceWeight){
    bool finished = false;
    float calibration_factors[5];
    float measured_values[5];
    if (referenceWeight <= 0) {
      referenceWeight = 100; // Default to 100g if invalid
    }

    for (int trial = 0; trial < 5; ++trial) {
      // Reset calibration factor to 1 before each trial
      calibration_factor = 1;
      scale.set_scale(calibration_factor);

      float measured;
      scale.get_units(measured, 2);
      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(": measured value = ");
      Serial.println(measured, 4);

      // Calibration loop: adjust calibration_factor so measured == 100
      Serial.println("Calibrating...");
      float tolerance = 0.05;
      int max_iterations = 1000;
      int iter = 0;
      float trial_calibration_factor = calibration_factor;

      while (abs(measured - referenceWeight) > tolerance && iter < max_iterations) {
        trial_calibration_factor *= (measured / referenceWeight); // Adjust factor proportionally
        scale.set_scale(trial_calibration_factor);
        scale.get_units(measured, 2);
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

    calibration_factor = avg_factor;
    EEPROM.put(CALIBRATION_FACTOR_ADDR, calibration_factor);
    EEPROM.commit();
    scale.set_scale(calibration_factor);
    finished = true;
    return finished;
}