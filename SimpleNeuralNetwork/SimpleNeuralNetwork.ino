#include "arduinoFFT.h"
#include <Arduino_LSM9DS1.h>

// FFT constants
#define SAMPLES 256
#define SAMPLING_FREQ 100
#define SAMPLING_PERIOD_MS (1000/SAMPLING_FREQ)
#define FEATURE_BINS 8
#define NUM_FEATURES (FEATURE_BINS + 3)  // Frequency bins + statistical features

// Create FFT object
ArduinoFFT<double> FFT;

// Arrays for FFT calculations
double vReal[SAMPLES];
double vImag[SAMPLES];

// Feature array
float features[NUM_FEATURES];
unsigned long millisOld;

// Frequency bands (Hz)
const float freqBands[FEATURE_BINS + 1] = {0, 5, 10, 15, 20, 25, 30, 40, 50};

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  
  FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);
  millisOld = millis();
  
  Serial.println("Send 1 or 0 to record and label a sample");
  // Print header for CSV format
  Serial.println("label,f0-5Hz,f5-10Hz,f10-15Hz,f15-20Hz,f20-25Hz,f25-30Hz,f30-40Hz,f40-50Hz,mean,max,variance");
}

void extractFeatures() {
  // Frequency bin energies
  for(int bin = 0; bin < FEATURE_BINS; bin++) {
    float binEnergy = 0;
    int startIndex = (freqBands[bin] * SAMPLES) / SAMPLING_FREQ;
    int endIndex = (freqBands[bin + 1] * SAMPLES) / SAMPLING_FREQ;
    
    for(int i = startIndex; i < endIndex; i++) {
      binEnergy += vReal[i];
    }
    features[bin] = binEnergy / (endIndex - startIndex);
  }
  
  // Statistical features
  float mean = 0, maxVal = 0, variance = 0;
  
  for(int i = 0; i < SAMPLES/2; i++) {
    mean += vReal[i];
    if(vReal[i] > maxVal) maxVal = vReal[i];
  }
  mean /= (SAMPLES/2);
  
  for(int i = 0; i < SAMPLES/2; i++) {
    variance += (vReal[i] - mean) * (vReal[i] - mean);
  }
  variance /= (SAMPLES/2);
  
  features[FEATURE_BINS] = mean;
  features[FEATURE_BINS + 1] = maxVal;
  features[FEATURE_BINS + 2] = sqrt(variance);
}

void collectAndProcessSample(int label) {
  float x, y, z;
  
  // Data Collection
  for(int i = 0; i < SAMPLES; i++) {
    while((millis() - millisOld) < SAMPLING_PERIOD_MS);
    millisOld = millis();
    
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(x, y, z);
      vReal[i] = x * 9.81;
    } else {
      vReal[i] = (i > 0) ? vReal[i-1] : 0;
    }
    vImag[i] = 0;
  }
  
  // FFT Processing
  FFT.dcRemoval();
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();
  
  extractFeatures();
  
  // Output single line in CSV format with label
  Serial.print(label);
  for(int i = 0; i < NUM_FEATURES; i++) {
    Serial.print(",");
    Serial.print(features[i], 6);
  }
  Serial.println();
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    
    if (input == '0' || input == '1') {
      int label = input - '0';
      collectAndProcessSample(label);
    }
  }
}