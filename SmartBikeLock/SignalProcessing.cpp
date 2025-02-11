#include "SignalProcessing.h"

const float SignalProcessing::freqBands[SignalConfig::FEATURE_BINS + 1] PROGMEM = {0, 6, 12, 19, 25, 31, 37, 44, 50};

SignalProcessing::SignalProcessing() 
    : FFT(vReal, vImag, SignalConfig::SAMPLES, SignalConfig::SAMPLING_FREQ), millisOld(0) {
    // Initialize arrays
    for(int i = 0; i < SignalConfig::SAMPLES; i++) {
        vReal[i] = 0;
        vImag[i] = 0;
    }
    for(int i = 0; i < SignalConfig::TOTAL_FEATURES; i++) {
        features[i] = 0;
    }
}

bool SignalProcessing::collectData() {
    float x, y, z;
    millisOld = millis();
    
    // Data Collection
    for(int i = 0; i < SignalConfig::SAMPLES; i++) {
        while((millis() - millisOld) < SignalConfig::SAMPLING_PERIOD_MS);
        millisOld = millis();
        
        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(x, y, z);
            vReal[i] = x * 9.81; // Convert to m/s^2
        } else {
            vReal[i] = (i > 0) ? vReal[i-1] : 0;
        }
        vImag[i] = 0;
    }
    return true;
}

void SignalProcessing::processData() {
    // FFT Processing
    FFT.dcRemoval();
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
    
    extractFeatures();
}

bool SignalProcessing::begin() {
    return IMU.begin();
}

void SignalProcessing::extractFeatures() {
    // Calculate frequency bin energies
    for(int bin = 0; bin < SignalConfig::FEATURE_BINS; bin++) {
        float binEnergy = 0;
        int startIndex = (pgm_read_float(&freqBands[bin]) * SignalConfig::SAMPLES) / SignalConfig::SAMPLING_FREQ;
        int endIndex = (pgm_read_float(&freqBands[bin + 1]) * SignalConfig::SAMPLES) / SignalConfig::SAMPLING_FREQ;
        
        for(int i = startIndex; i < endIndex; i++) {
            binEnergy += vReal[i];
        }
        features[bin] = binEnergy / (endIndex - startIndex);
    }
    
    // Calculate statistical features
    float mean = 0, maxVal = 0;
    for(int i = 0; i < SignalConfig::SAMPLES/2; i++) {
        mean += vReal[i];
        if(vReal[i] > maxVal) maxVal = vReal[i];
    }
    mean /= (SignalConfig::SAMPLES/2);
    
    float variance = 0;
    for(int i = 0; i < SignalConfig::SAMPLES/2; i++) {
        float diff = vReal[i] - mean;
        variance += diff * diff;
    }
    variance /= (SignalConfig::SAMPLES/2);
    
    // Store statistical features
    features[SignalConfig::FEATURE_BINS] = mean;
    features[SignalConfig::FEATURE_BINS + 1] = maxVal;
    features[SignalConfig::FEATURE_BINS + 2] = sqrt(variance);
}