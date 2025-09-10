#include <Arduino.h>
#include <scale.h>

#define WINDOW_SIZE 5
float sampleBuffer[WINDOW_SIZE];
int sampleIndex = 0;
bool bufferFilled = false;

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
    float nyVerdi = updateScale();

    // Put value in buffer and calculate median value
    sampleBuffer[sampleIndex] = nyVerdi;
    sampleIndex++;
     if (sampleIndex >= WINDOW_SIZE) {
      sampleIndex = 0;
     bufferFilled = true;
     }
     int samples = bufferFilled ? WINDOW_SIZE : sampleIndex;
     float medianValue = getMedian(sampleBuffer, samples);

    // Exponential smoothing
    static float filteredWeight = 0;
    float alpha = 0.7; // Higher alpha gives better response time but more noise, vice versa
    filteredWeight = alpha * medianValue + (1 - alpha) * filteredWeight;
    if (filteredWeight > -0.09 && filteredWeight < 0.09) {
      filteredWeight = 0;
    }
    return filteredWeight;
}