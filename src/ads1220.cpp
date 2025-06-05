#include <Arduino.h>
#include <ADS1220.hpp>


// ---------- Pin definitions (ESP32-S3) ----------
#define PIN_SCK    5    // SPI clock
#define PIN_MISO   4    // DOUT/DRDY from ADS1220 (also used as DRDY)
#define PIN_MOSI   7    // DIN to ADS1220
#define PIN_CS     6    // Chip Select
#define PIN_DRDY   8    // Same as MISO; used for DRDY signal
#define PIN_TARE   3
#define PGA 128               // Programmable Gain, confirm that the same as set_pga_gain
#define VREF 2.048            // Internal reference of 2.048V
#define VFSR VREF/PGA
#define FSR (((long int)1<<23)-1)

#define ADS_SCALE 1081.081f

#define LOWER_BOUNDS

// ---------- Calibration constants (adjust for your load cell) ----------
#define LOAD_CELL_ZERO_OFFSET   0       // Raw ADC value at zero load
#define LOAD_CELL_SCALE         1.0f    // Scale factor (grams per raw unit)

ads1220::ADS1220 ads;
volatile bool drdyIntrFlag = false;
volatile int tareCounter = -1;
float Vout = 0;
int32_t adc_data = 0;
float weight = 0;
float ADS_LOW_OFFSET = 0.0085f;

void drdyInterruptHndlr(){
  drdyIntrFlag = true;
}

void tareInterruptHandler() {
  tareCounter = 4;
}

void enableInterruptPin(){
  attachInterrupt(digitalPinToInterrupt(PIN_DRDY), drdyInterruptHndlr, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TARE), tareInterruptHandler, FALLING);
}

void setup_ads() {
  Serial.println("Starting ads setup");

  // 1) Initialize the SPI bus (ESP32-S3 allows any GPIO pins)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  SPISettings spi_settings(1000000, MSBFIRST, SPI_MODE1);
  ads.begin(SPI, PIN_CS, PIN_DRDY, spi_settings);
  using namespace ads1220::config;

  ads.write_config_pga_bypass(pga_bypass::DISABLE);
  ads.write_config_pga_gain(gain::GAIN_128);
  ads.write_config_input_multiplexer(mux::AIN1_AIN2);
  ads.write_config_burn_out_current_sources(burn_out_current_sources::OFF);
  ads.write_config_temperature_sensor(temperature_sensor::DISABLE);
  ads.write_config_conversion_mode(conversion_mode::CONTINUOUS);
  ads.write_config_operating_mode(mode::NORMAL);
  ads.write_config_data_rate_normal(data_rate::NORMAL_330_SPS);
  ads.write_config_idac_current(idac::IDAC_500_UA);
  ads.write_config_low_side_power_switch(low_side_power_switch::AUTO_CLOSE);
  ads.write_config_fir_filter(fir_50_60::FIR_50HZ_60HZ);
  ads.write_config_vref_selection(vref::INTERNAL_2_048_V);
  ads.write_config_drdy_mode(drdy_mode::DRDY_ONLY);
  ads.write_config_idac2_routing(i2mux::AIN0);
  ads.write_config_idac1_routing(i1mux::DISABLE);

  ads.print_config_registers();
  ads.begin_continuous();


  enableInterruptPin();

  Serial.println("setup ads complete");
  // Allow some time for the ADC to begin conversions
  delay(100);
}

float get_weight() {
  for (int i = 0; i < 50; i++) {
    delay(5);
    adc_data=ads.read();
    if (adc_data != 0 && adc_data != 0xffffffff) {
      Vout = (float)((adc_data*VFSR*1000)/FSR);     //In  mV
      if (Vout < 1 && Vout > 0) {
        float newWeight = (Vout - ADS_LOW_OFFSET) * ADS_SCALE;
        weight = weight * 0.95f + newWeight * 0.05f;
      }
    }
  }
  if (tareCounter >= 0) {
    tareCounter--;
    if (tareCounter == 0) {
      ADS_LOW_OFFSET = Vout;
      weight = 0.0f;
      printf("Tare: Vout=%.08f\n", Vout);
    }
  }

  printf("ADS Measurement: Weight=%.02f Vout=%.08f, Raw=%04x\n", weight, Vout, adc_data);
  return weight;
}
