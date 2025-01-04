#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "arduinoFFT.h"
#include <Arduino_LSM9DS1.h>
#include "Config.h"

class SignalProcessing {
public:
    SignalProcessing();
    
    bool begin();
    bool collectData();
    void processData();
    const float* getFeatures() const { return features; }
    
private:
    ArduinoFFT<float> FFT;
    float vReal[SignalConfig::SAMPLES];
    float vImag[SignalConfig::SAMPLES];
    float features[SignalConfig::TOTAL_FEATURES];
    unsigned long millisOld;
    
    static const float freqBands[SignalConfig::FEATURE_BINS + 1] PROGMEM;
    void extractFeatures();
};

#endif