#include <HX711.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <algorithm>

#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN   3
#define LOADCELL_POWER_PIN 6

// ---- Tuning knobs ---------------------------------------------------------
constexpr int   TARE_SAMPLES             = 15;     // total raw samples collected
constexpr int   TARE_DISCARD_EACH_SIDE   = 3;      // trimmed-mean: drop N lowest + N highest
constexpr int   TARE_WARMUP_SAMPLES      = 3;      // discard first reads (HX711 settle)
constexpr unsigned long TARE_SAMPLE_TIMEOUT_MS = 200;
constexpr long  TARE_MAX_OFFSET_JUMP     = 500000; // raw counts; sanity vs previous offset
constexpr float WEIGHT_EMA_ALPHA         = 0.4f;
constexpr unsigned long WEIGHT_EXTRAPOLATE_CAP_MS = 50;
// --------------------------------------------------------------------------

float calibration_factor = 1; // Default value
const int CALIBRATION_FACTOR_ADDR = 0; // EEPROM address
bool isCalibrating = false;

HX711 scale;

// ---- Interpolated weight state -------------------------------------------
static float         g_emaWeight     = 0.0f;
static bool          g_emaInit       = false;
static float         g_lastWeight    = 0.0f;
static unsigned long g_lastWeightTs  = 0;
static float         g_prevWeight    = 0.0f;
static unsigned long g_prevWeightTs  = 0;


float getOffset(){
  return scale.get_offset();
}

bool setOffset(float newOffset){
  scale.set_offset(newOffset);
  return true;
}

// Robust tare: warm-up discard + trimmed-mean of N samples + sanity check.
bool tareScale(){
  const long prevOffset = scale.get_offset();

  // Warm-up: throw away stale/first samples.
  for (int i = 0; i < TARE_WARMUP_SAMPLES; ++i) {
    if (!scale.wait_ready_timeout(TARE_SAMPLE_TIMEOUT_MS)) return false;
    (void)scale.read();
  }

  long samples[TARE_SAMPLES];
  for (int i = 0; i < TARE_SAMPLES; ++i) {
    if (!scale.wait_ready_timeout(TARE_SAMPLE_TIMEOUT_MS)) return false;
    samples[i] = scale.read();
  }

  // Trimmed mean: sort, drop tails, average the middle.
  std::sort(samples, samples + TARE_SAMPLES);
  const int lo = TARE_DISCARD_EACH_SIDE;
  const int hi = TARE_SAMPLES - TARE_DISCARD_EACH_SIDE;
  long long sum = 0;
  for (int i = lo; i < hi; ++i) sum += samples[i];
  const long avg = (long)(sum / (hi - lo));

  // Sanity: if we already had a sane offset and the new one jumps absurdly,
  // reject and try once more.
  if (prevOffset != 0 && labs(avg - prevOffset) > TARE_MAX_OFFSET_JUMP) {
    long samples2[TARE_SAMPLES];
    for (int i = 0; i < TARE_SAMPLES; ++i) {
      if (!scale.wait_ready_timeout(TARE_SAMPLE_TIMEOUT_MS)) return false;
      samples2[i] = scale.read();
    }
    std::sort(samples2, samples2 + TARE_SAMPLES);
    long long sum2 = 0;
    for (int i = lo; i < hi; ++i) sum2 += samples2[i];
    const long avg2 = (long)(sum2 / (hi - lo));
    if (labs(avg2 - prevOffset) > TARE_MAX_OFFSET_JUMP) return false;
    scale.set_offset(avg2);
  } else {
    scale.set_offset(avg);
  }

  // Reset interpolator so display snaps to zero cleanly.
  g_emaInit      = false;
  g_lastWeight   = 0.0f;
  g_prevWeight   = 0.0f;
  g_lastWeightTs = millis();
  g_prevWeightTs = g_lastWeightTs;
  return true;
}

void setupScale(){
  EEPROM.begin(512);
  EEPROM.get(CALIBRATION_FACTOR_ADDR, calibration_factor);
  if (isnan(calibration_factor) || calibration_factor == 0) {
    calibration_factor = 1; // Reset to default if invalid
  }
  pinMode(LOADCELL_POWER_PIN, OUTPUT);
  digitalWrite(LOADCELL_POWER_PIN, HIGH);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_gain();
  scale.set_scale(calibration_factor);
  tareScale();
}

// Non-blocking sampler: call from loop() as often as you like.
void tickScale(){
  if (!scale.is_ready()) return;

  const float raw = scale.get_units(1);
  if (!g_emaInit) {
    g_emaWeight = raw;
    g_emaInit   = true;
  } else {
    g_emaWeight = WEIGHT_EMA_ALPHA * raw + (1.0f - WEIGHT_EMA_ALPHA) * g_emaWeight;
  }

  g_prevWeight   = g_lastWeight;
  g_prevWeightTs = g_lastWeightTs;
  g_lastWeight   = g_emaWeight;
  g_lastWeightTs = millis();
}

// Time-interpolated weight between the last two HX711 frames.
float getWeight(){
  if (!g_emaInit) return 0.0f;
  const unsigned long dt = g_lastWeightTs - g_prevWeightTs;
  if (dt == 0) return g_lastWeight;

  unsigned long since = millis() - g_lastWeightTs;
  if (since > WEIGHT_EXTRAPOLATE_CAP_MS) since = WEIGHT_EXTRAPOLATE_CAP_MS;

  const float slope = (g_lastWeight - g_prevWeight) / (float)dt;
  return g_lastWeight + slope * (float)since;
}

// Back-compat shim for existing callers.
float updateScale(){
  return getWeight();
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

      float measured = scale.get_units(2);
      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(": measured value = ");
      Serial.println(measured, 4);

      // Calibration loop: adjust calibration_factor so measured == reference
      Serial.println("Calibrating...");
      float tolerance = 0.05;
      int max_iterations = 1000;
      int iter = 0;
      float trial_calibration_factor = calibration_factor;

      while (abs(measured - referenceWeight) > tolerance && iter < max_iterations) {
        trial_calibration_factor *= (measured / referenceWeight);
        scale.set_scale(trial_calibration_factor);
        measured = scale.get_units(2);
        iter++;
      }

      calibration_factors[trial] = trial_calibration_factor;
      measured_values[trial]     = measured;

      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(" calibration factor: ");
      Serial.println(trial_calibration_factor, 6);
      Serial.print("Trial ");
      Serial.print(trial + 1);
      Serial.print(" measured value: ");
      Serial.println(measured, 4);

      delay(1000);
    }

    float sum_factor = 0;
    float sum_measured = 0;
    for (int i = 0; i < 5; ++i) {
      sum_factor   += calibration_factors[i];
      sum_measured += measured_values[i];
    }
    float avg_factor   = sum_factor   / 5.0;
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
