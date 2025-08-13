#include <Arduino.h>
#include <ADS1220.hpp>

// ---------- Pin definitions (ESP32-S3) ----------
#define PIN_SCK    5    // SPI clock
#define PIN_MISO   4    // DOUT/DRDY from ADS1220 (also used as DRDY)
#define PIN_MOSI   7    // DIN to ADS1220
#define PIN_CS     6    // Chip Select
#define PIN_DRDY   8    // Same as MISO; used for DRDY signal
#define PIN_TARE   3

// ---------- ADS1220 scaling & calibration ----------
#define PGA                   128               // actual PGA gain configured
#define VREF                  3.300f           // using AVDD as reference
#define VFSR                  (VREF / PGA)     // full-scale input voltage (V)
#define FSR                   (((int32_t)1<<23) - 1)
#define ADS_SCALE             1081.081f        // your grams-per-mV scale
float ADS_LOW_OFFSET = 0.0085f;                // mV offset from tare

// Tiny weights under this (grams) clamp to zero:
const float MIN_WEIGHT_THRESHOLD = 0.05f;

// Smoothing factor for exponential filter:
const float SMOOTHING_FACTOR = 0.05f;

// ---------- Globals ----------
ads1220::ADS1220 ads;
SPIClass        spiAds(1);
volatile bool   drdyIntrFlag = false;
volatile int    tareCounter   = -1;
float           weight        = 0;
float           Vout          = 0.0f;

// ---------- ISRs & Interrupt setup ----------
void IRAM_ATTR drdyInterruptHandler() {
  drdyIntrFlag = true;
}

void IRAM_ATTR tareInterruptHandler() {
  tareCounter = 4;  // average next 4 samples for tare
}

void enableInterruptPin() {
  attachInterrupt(digitalPinToInterrupt(PIN_DRDY),
                  drdyInterruptHandler,
                  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TARE),
                  tareInterruptHandler,
                  FALLING);
}

// ---------- Setup & Scale init ----------
void setupScale() {
  Serial.println("Starting scale setup");

  // SPI + ADS1220 init
  spiAds.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE1);
  ads.begin(spiAds, PIN_CS, PIN_DRDY, spiSettings);

  using namespace ads1220::config;
  ads.write_config_pga_bypass(pga_bypass::DISABLE);
  ads.write_config_pga_gain(gain::GAIN_128);
  ads.write_config_input_multiplexer(mux::AIN1_AIN2);
  ads.write_config_burn_out_current_sources(burn_out_current_sources::OFF);
  ads.write_config_temperature_sensor(temperature_sensor::DISABLE);
  ads.write_config_conversion_mode(conversion_mode::CONTINUOUS);
  ads.write_config_operating_mode(mode::TURBO);
  ads.write_config_data_rate_turbo(data_rate::TURBO_90_SPS);
  ads.write_config_idac_current(idac::IDAC_500_UA);
  ads.write_config_low_side_power_switch(low_side_power_switch::AUTO_CLOSE);
  ads.write_config_fir_filter(fir_50_60::FIR_50HZ_60HZ);
  ads.write_config_vref_selection(vref::ANALOG_AVDD_AVSS);
  ads.write_config_drdy_mode(drdy_mode::DRDY_ONLY);
  ads.write_config_idac2_routing(i2mux::DISABLE);
  ads.write_config_idac1_routing(i1mux::AIN0);

  ads.print_config_registers();
  ads.begin_continuous();

  enableInterruptPin();

  Serial.println("Scale setup complete");
  delay(100);  // let the ADC settle
}

// ---------- Main sample & conversion ----------
float updateScale() {
  // 1) No new data? return last weight.
  if (!drdyIntrFlag) {
    return weight;
  }
  drdyIntrFlag = false;

  // 2) Read fresh conversion
  int32_t raw = ads.read();
  if (raw == 0 || raw == 0xFFFFFF) {
    return weight;  // skip invalid codes
  }

  // 3) Convert raw → mV
  Vout = (float)raw * (VFSR * 1000.0f) / (float)FSR;

  // 4) Instantaneous weight (mV → grams)
  float newWeight = (Vout - ADS_LOW_OFFSET) * ADS_SCALE;

  // 6) Clamp tiny unload drift to zero
  if (weight > 0 && weight < MIN_WEIGHT_THRESHOLD) {
    weight = 0;
  }

  // 7) Tare handling
  if (tareCounter >= 0) {
    if (--tareCounter == 0) {
      ADS_LOW_OFFSET = Vout;
      weight         = 0.0f;
      Serial.printf("Tare complete: Vout=%.6f mV\n", Vout);
      tareCounter = -1;
    }
  }

  return newWeight;
}