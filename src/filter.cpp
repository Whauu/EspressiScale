#include <arduino.h>
#include <ads1220.h>

#define WINDOW_SIZE 5
float sampleBuffer[WINDOW_SIZE];
int sampleIndex = 0;
bool bufferFilled = false;
const float ZERO_ENTER  = 0.08f;  // below this → force zero
const float ZERO_EXIT   = 0.12f;  // above this → allow non-zero

// Function for finding median value in array
float getMedian(float arr[], int n) {
  float sorted[WINDOW_SIZE];
  for (int i = 0; i < n; i++) {
    sorted[i] = arr[i];
  }
  // Use simple bubble sort to sort the array
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (sorted[j] < sorted[i]) {
         float temp = sorted[i];
         sorted[i] = sorted[j];
         sorted[j] = temp;
      }
    }
  }
  if (n % 2 == 1) {
    return sorted[n / 2];
  } else {
    return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
  }
}

float medianFilter(){
    // 1) grab raw, medianize
    float sample = updateScale();
    sampleBuffer[sampleIndex++] = sample;
    if (sampleIndex == WINDOW_SIZE) {
      sampleIndex = 0;
      bufferFilled = true;
    }
    int count = bufferFilled ? WINDOW_SIZE : sampleIndex;
    float med = getMedian(sampleBuffer, count);

    // 2) exponential smoothing, dual-alpha
    static float filtered = 0.0f;
    const float ALPHA_RISE = 0.7f;
    const float ALPHA_FALL = 0.3f;
    float alpha = (med > filtered) ? ALPHA_RISE : ALPHA_FALL;
    filtered = alpha * med + (1 - alpha) * filtered;

    // 3) hysteresis dead-band around zero
    // if we’re already zero, only leave zero when > ZERO_EXIT
    // if we’re non-zero, only go to zero when < ZERO_ENTER
    static bool isZero = true;
    if (isZero) {
      if (filtered > ZERO_EXIT) {
        isZero = false;
      }
    } else {
      if (fabs(filtered) < ZERO_ENTER) {
        isZero = true;
      }
    }
    return isZero ? 0.0f : filtered;
}