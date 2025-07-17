/*#include <HX711.h>
#include <Arduino.h>
#include <EEPROM.h>

#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN   3
#define LOADCELL_POWER_PIN 6

float calibration_factor; // Default value
const int CALIBRATION_FACTOR_ADDR = 0; // EEPROM address

void setupCalibrationFactor() {
  EEPROM.begin(512);
  EEPROM.get(CALIBRATION_FACTOR_ADDR, calibration_factor);

  // Check if calibration_factor is uninitialized (e.g., NaN or out of expected range)
  if (isnan(calibration_factor) || calibration_factor < 0.1f || calibration_factor > 100000.0f) {
    Serial.begin(921600);
    while (!Serial) { delay(10); }
    Serial.println("Enter calibration factor:");
    while (Serial.available() == 0) { delay(10); }
    calibration_factor = Serial.parseFloat();
    EEPROM.put(CALIBRATION_FACTOR_ADDR, calibration_factor);
    EEPROM.commit();
    Serial.print("Calibration factor set to: ");
    Serial.println(calibration_factor);
  } else {
    Serial.begin(921600);
    Serial.print("Loaded calibration factor from EEPROM: ");
    Serial.println(calibration_factor);
  }
}

HX711 scale;

void setupScale(){
  pinMode(LOADCELL_POWER_PIN, OUTPUT);
  digitalWrite(LOADCELL_POWER_PIN, HIGH);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_gain();
  scale.set_scale(calibration_factor);
  scale.tare(); 
}

void reTareScale(){
  scale.tare();
}

void tareScale(){
  //delay(500);
  int times = 20;
	long sum = 0;
  long lastSum = 0;
  boolean finished = false;
  int stableCounter = 0;

	for (byte i = 0; i < times && !finished; i++) {
		sum = scale.read();
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
}
float updateScale(){
  return scale.get_units();
}*/