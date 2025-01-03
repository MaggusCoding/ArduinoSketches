#include "arduinoFFT.h"
#include "SignalProcessing.h"


float *output;

// Arrays for FFT calculations
float vReal[SAMPLES];    // Changed to float from double
float vImag[SAMPLES];    // Changed to float from double
float features[11];      // 8 frequency bins + 3 statistical features
unsigned long millisOld;

// Frequency Bands
const PROGMEM float freqBands[FEATURE_BINS + 1] = {0, 5, 10, 15, 20, 25, 30, 40, 50};

void SignalProcessing::innit(float vReal, float vImag, float SAMPLES, unsigned int SAMPLING_FREQ){

}