#include "arduinoFFT.h"
#include <Arduino_LSM9DS1.h>
// FFT constants
#define SAMPLES 256
#define SAMPLING_FREQ 100
#define SAMPLING_PERIOD_MS (1000/SAMPLING_FREQ)
#define FEATURE_BINS 8

ArduinoFFT<float> FFT;  // Changed to float from double to save memory
// Arrays for FFT calculations
float vReal[SAMPLES];    // Changed to float from double
float vImag[SAMPLES];    // Changed to float from double
float features[11];      // 8 frequency bins + 3 statistical features
unsigned long millisOld;
unsigned long startTime;  // Added to track start of data collection
float x, y, z;

const float freqBands[FEATURE_BINS + 1] = {0, 5, 10, 15, 20, 25, 30, 40, 50};

void extractFeatures() {
    // Compute FFT
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();
    
    // Frequency bin energies
    for(int bin = 0; bin < FEATURE_BINS; bin++) {
        float binEnergy = 0;
        int startIndex = (pgm_read_float(&freqBands[bin]) * SAMPLES) / SAMPLING_FREQ;
        int endIndex = (pgm_read_float(&freqBands[bin + 1]) * SAMPLES) / SAMPLING_FREQ;
        
        for(int i = startIndex; i < endIndex; i++) {
            binEnergy += vReal[i];
        }
        features[bin] = binEnergy / (endIndex - startIndex);
    }
    
    // Statistical features using running calculations to save memory
    float mean = 0, maxVal = 0;
    for(int i = 0; i < SAMPLES/2; i++) {
        mean += vReal[i];
        if(vReal[i] > maxVal) maxVal = vReal[i];
    }
    mean /= (SAMPLES/2);
    
    float variance = 0;
    for(int i = 0; i < SAMPLES/2; i++) {
        float diff = vReal[i] - mean;
        variance += diff * diff;
    }
    variance /= (SAMPLES/2);
    
    features[FEATURE_BINS] = mean;
    features[FEATURE_BINS + 1] = maxVal;
    features[FEATURE_BINS + 2] = sqrt(variance);
}

void setup() {
    Serial.begin(115200);
    while(!Serial);
    
    if (!IMU.begin()) {
        Serial.println("Failed to initialize IMU!");
        while (1);
    }
    
    FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES, SAMPLING_FREQ);
    millisOld = millis();
    
}

void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == 's' || input == 'S') {
            startTime = millis();  // Record start time when 's' is pressed
            Serial.println("TimeElapsed(ms),AccelX(m/s^2),AccelY(m/s^2),AccelZ(m/s^2)");
            // Collect samples for FFT
            for(int i = 0; i < SAMPLES; i++) {
                while((millis() - millisOld) < SAMPLING_PERIOD_MS);
                unsigned long currentTime = millis();
                millisOld = currentTime;
                
                if (IMU.accelerationAvailable()) {
                    IMU.readAcceleration(x, y, z);
                    // Store acceleration data for FFT
                    vReal[i] = x * 9.81;
                    vImag[i] = 0;
                    
                    // Print elapsed time and raw data
                    Serial.print(currentTime - startTime);  // Elapsed time
                    Serial.print(",");
                    Serial.print(x * 9.81, 6);
                    Serial.println();
                } else {
                    vReal[i] = (i > 0) ? vReal[i-1] : 0;
                    vImag[i] = 0;
                }
            }
            
            // Calculate FFT features
            extractFeatures();
            
            // Display features
            Serial.println("\nFeature values for this sample:");
            for(int i = 0; i < FEATURE_BINS; i++) {
                Serial.print("Freq ");
                Serial.print(pgm_read_float(&freqBands[i]));
                Serial.print("-");
                Serial.print(pgm_read_float(&freqBands[i+1]));
                Serial.print("Hz: ");
                Serial.println(features[i], 6);
            }
        }
    }
}