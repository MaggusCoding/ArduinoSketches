#define NumberOf(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))
//#define _2_OPTIMIZE 0B00001000 //Int Quanitzation
#define _1_OPTIMIZE 0B00010000
//#define _2_OPTIMIZE 0B01000000 // No Biases

#include <NeuralNetwork.h>
#include "arduinoFFT.h"
#include <Arduino_LSM9DS1.h>

// FFT constants
#define SAMPLES 256
#define SAMPLING_FREQ 100
#define SAMPLING_PERIOD_MS (1000/SAMPLING_FREQ)
#define FEATURE_BINS 8

// Neural Network and FFT objects
NeuralNetwork *NN;  
float *output;      
ArduinoFFT<float> FFT;  // Changed to float from double to save memory

// Arrays for FFT calculations
float vReal[SAMPLES];    // Changed to float from double
float vImag[SAMPLES];    // Changed to float from double
float features[11];      // 8 frequency bins + 3 statistical features
unsigned long millisOld;

// Frequency bands (Hz) - moved to PROGMEM to save RAM
const PROGMEM float freqBands[FEATURE_BINS + 1] = {0, 5, 10, 15, 20, 25, 30, 40, 50};

// Network architecture
const unsigned int layers[] = {11, 20, 1};

// Training data moved to PROGMEM
const PROGMEM float inputs[16][11] = {
    // Format: f0-5Hz,f5-10Hz,f10-15Hz,f15-20Hz,f20-25Hz,f25-30Hz,f30-40Hz,f40-50Hz,mean,max,variance
    {57.928509,14.470326,8.354366,4.072833,2.259012,2.115621,1.281847,1.137330,9.081741,162.061386,20.795198},
    {0.249636,0.310526,0.284251,0.289740,0.276506,0.293029,0.275930,0.285157,0.282762,0.590756,0.083509},
    {40.523720,14.090983,11.033436,5.623566,3.171416,2.047416,1.925643,1.129067,8.056471,79.404900,12.813414},
    {0.092623,0.091632,0.147428,0.062245,0.073431,0.047236,0.038493,0.050569,0.069262,0.294503,0.055974},
    {50.662792,22.568745,12.957399,6.475854,3.548554,3.598305,2.184609,1.502175,10.462087,97.633827,16.766186},
    {0.192281,0.201777,0.132999,0.078350,0.067252,0.057038,0.052922,0.044456,0.091942,0.479395,0.089565},
    {47.318878,24.611332,5.407272,3.697110,3.413770,2.539831,1.273825,1.078464,8.923025,116.080383,16.665312},
    {0.096454,0.077746,0.091129,0.032084,0.041571,0.028106,0.035343,0.026464,0.048864,0.288771,0.048027},
    {36.649235,14.464705,9.048715,3.719692,3.130369,1.862029,1.483838,0.957352,7.190089,118.930450,14.377792},
    {0.150395,0.195433,0.131368,0.051687,0.052537,0.041452,0.052452,0.047448,0.082054,0.858473,0.104018},
    {31.517038,13.517478,13.759942,2.860543,2.029840,2.100289,1.336743,1.164108,6.926655,67.461426,11.004887},
    {2.016347,2.704451,4.680589,3.015826,2.751961,2.217993,1.735959,0.939064,2.276168,11.290654,1.567029},
    {41.681168,11.800854,8.496819,3.517949,1.820635,1.602628,1.353350,1.173330,7.174771,179.494263,18.839750},
    {1.715903,2.167234,2.968738,1.863114,1.737162,2.312426,1.199850,1.066952,1.725375,7.378445,1.031108}
};

const PROGMEM float expectedOutput[16][1] = {
    {1}, {0}, {1}, {0}, {1}, {0}, {1}, {0}, 
    {1}, {0}, {1}, {0}, {1}, {0}
};

// Test data moved to PROGMEM
const PROGMEM float testInputs[6][11] = {
    {48.757507,13.254408,15.007956,3.677550,2.028057,2.463273,1.520370,0.720055,8.706903,200.533569,22.367250},
    {1.458818,3.667517,1.585120,0.979926,1.155209,0.889905,0.669370,0.583588,1.225020,10.401380,1.381269},
    {41.665482,7.996378,5.524218,3.894002,2.308299,3.325120,1.562609,0.928244,6.726930,119.694992,14.722657},
    {0.109169,0.112447,0.118559,0.072998,0.079882,0.056962,0.059612,0.056215,0.078091,0.337020,0.056511},
    {67.647438,13.110575,5.397443,3.602028,3.905321,3.066864,1.308789,1.110925,9.763156,153.393173,21.911320},
    {0.463661,0.273754,0.094462,0.100464,0.097789,0.067037,0.063633,0.064684,0.133349,1.555442,0.199772}
};

const PROGMEM float testExpectedOutput[6][1] = {
    {1}, {0}, {1}, {0}, {1}, {0}
};

// Buffer for copying from PROGMEM
float inputBuffer[11];
float outputBuffer[1];

void setup() {
    Serial.begin(115200);
    while(!Serial);
    
    if (!IMU.begin()) {
        Serial.println("Failed to initialize IMU!");
        while (1);
    }
    
    FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES, SAMPLING_FREQ);
    millisOld = millis();
    
    Serial.println("Starting neural network training...");
    
    NN = new NeuralNetwork(layers, NumberOf(layers));
    unsigned int epoch = 0;
    float currentMSE;
    
    do {
        epoch++;
        
        for (unsigned int j = 0; j < NumberOf(inputs); j++) {
            // Copy input data from PROGMEM to RAM
            for(int k = 0; k < 11; k++) {
                inputBuffer[k] = pgm_read_float(&inputs[j][k]);
            }
            outputBuffer[0] = pgm_read_float(&expectedOutput[j][0]);
            
            NN->FeedForward(inputBuffer);      
            NN->BackProp(outputBuffer); 
        }
        
        currentMSE = NN->getMeanSqrdError(NumberOf(inputs));
        
        if(epoch % 10 == 0) {
            Serial.print("Epoch ");
            Serial.print(epoch);
            Serial.print(" - MSE: "); 
            Serial.println(currentMSE, 6);
        }
        
        if(epoch >= 1000) {
            Serial.println("Maximum epochs reached!");
            break;
        }
    } while(currentMSE > 0.01);

    Serial.println("\nTraining complete!");
    Serial.println("\nSend 't' for live training or 's' for classification");
}

void extractFeatures() {
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

void performLiveClassification() {
    Serial.println("Starting data collection...");
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
    
    FFT.dcRemoval();
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
    
    extractFeatures();
    
    output = NN->FeedForward(features);
    float prediction = (output[0] > 0.5) ? 1 : 0;
    
    Serial.print("Classification: ");
    Serial.println(prediction == 1 ? "SUSPICIOUS" : "NORMAL");
    Serial.print("Confidence: ");
    Serial.print(abs(output[0] - 0.5) * 200, 1);
    Serial.println("%");
    
    Serial.println("\nSend 't' for live training or 's' for classification");
}

void performLiveTraining() {
    Serial.println("Starting data collection for training...");
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
    
    // Extract features
    extractFeatures();
    
    // Display features and ask for label
    Serial.println("\nFeature values for this sample:");
    for(int i = 0; i < FEATURE_BINS; i++) {
        Serial.print("Freq ");
        Serial.print(pgm_read_float(&freqBands[i]));
        Serial.print("-");
        Serial.print(pgm_read_float(&freqBands[i+1]));
        Serial.print("Hz: ");
        Serial.println(features[i], 6);
    }
    Serial.print("Mean: "); Serial.println(features[FEATURE_BINS], 6);
    Serial.print("Max: "); Serial.println(features[FEATURE_BINS + 1], 6);
    Serial.print("Variance: "); Serial.println(features[FEATURE_BINS + 2], 6);
    
    // Wait for label input
    Serial.println("\nEnter label (0 or 1):");
    float label = -1;
    while(label == -1) {
        if(Serial.available() > 0) {
            char input = Serial.read();
            if(input == '0' || input == '1') {
                label = input - '0';
                
                // First perform forward pass
                NN->FeedForward(features);
                
                // Then perform backpropagation with the provided label
                float expectedOutput[] = {label};
                NN->BackProp(expectedOutput);
                
                // Get current prediction and error
                output = NN->FeedForward(features);
                float error = label - output[0];
                
                // Display training results
                Serial.print("\nSample trained with label: ");
                Serial.println(label);
                Serial.print("Network output after training: ");
                Serial.print(output[0], 4);
                Serial.print(" (error: ");
                Serial.print(abs(error), 4);
                Serial.println(")");
            }
        }
    }
    
    Serial.println("\nSend 't' for live training or 's' for classification");
}

// Modify loop to include live training option
void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == 's' || input == 'S') {
            performLiveClassification();
        }
        else if (input == 't' || input == 'T') {
            performLiveTraining();
        }
    }
}