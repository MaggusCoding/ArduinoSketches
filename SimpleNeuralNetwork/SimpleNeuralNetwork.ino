#define NumberOf(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))
#define _2_OPTIMIZE 0B00000000
#define _1_OPTIMIZE 0B00010000

#include <NeuralNetwork.h>

// Global variables
NeuralNetwork *NN;  
float *output;      

// Network architecture: 11 inputs (8 freq bins + 3 stats), 40 and 30 hidden neurons, 1 output
const unsigned int layers[] = {11, 40, 30, 1};

// Training data (16 samples)
const float inputs[16][11] = {
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

const float expectedOutput[16][1] = {
    {1}, {0}, {1}, {0}, {1}, {0}, {1}, {0}, 
    {1}, {0}, {1}, {0}, {1}, {0}
};

// Test data (6 samples)
const float testInputs[6][11] = {
    {48.757507,13.254408,15.007956,3.677550,2.028057,2.463273,1.520370,0.720055,8.706903,200.533569,22.367250},
    {1.458818,3.667517,1.585120,0.979926,1.155209,0.889905,0.669370,0.583588,1.225020,10.401380,1.381269},
    {41.665482,7.996378,5.524218,3.894002,2.308299,3.325120,1.562609,0.928244,6.726930,119.694992,14.722657},
    {0.109169,0.112447,0.118559,0.072998,0.079882,0.056962,0.059612,0.056215,0.078091,0.337020,0.056511},
    {67.647438,13.110575,5.397443,3.602028,3.905321,3.066864,1.308789,1.110925,9.763156,153.393173,21.911320},
    {0.463661,0.273754,0.094462,0.100464,0.097789,0.067037,0.063633,0.064684,0.133349,1.555442,0.199772}
};

const float testExpectedOutput[6][1] = {
    {1}, {0}, {1}, {0}, {1}, {0}
};

// Timing variables
unsigned long inferenceTime = 0;
unsigned long trainingTime = 0;
unsigned long totalInferenceTime = 0;
unsigned long totalTrainingTime = 0;
unsigned long startTime = 0;

void setup() {
    Serial.begin(115200);
    while(!Serial);
    
    Serial.println("Starting training with FFT data...");
    
    NN = new NeuralNetwork(layers, NumberOf(layers));
    unsigned int epoch = 0;
    float currentMSE;
    
    do {
        epoch++;
        totalInferenceTime = 0;
        totalTrainingTime = 0;
        
        for (unsigned int j = 0; j < NumberOf(inputs); j++) {
            // Measure pure inference time
            startTime = micros();
            NN->FeedForward(inputs[j]);      
            inferenceTime = micros() - startTime;
            totalInferenceTime += inferenceTime;
            
            // Measure complete training iteration time
            startTime = micros();
            NN->FeedForward(inputs[j]);      
            NN->BackProp(expectedOutput[j]); 
            trainingTime = micros() - startTime;
            totalTrainingTime += trainingTime;
        }
        
        currentMSE = NN->getMeanSqrdError(NumberOf(inputs));
        
        if(epoch % 100 == 0) {  // Print every 100 epochs
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

    Serial.println("\n=-[TRAINING COMPLETE]-=");
    
    // Test the network
    float correctPredictions = 0;
    float totalMSE = 0;
    
    Serial.println("\n=-[TEST RESULTS]-=");
    
    for (unsigned int i = 0; i < NumberOf(testInputs); i++) {
        output = NN->FeedForward(testInputs[i]);
        
        float error = testExpectedOutput[i][0] - output[0];
        totalMSE += error * error;
        
        float prediction = (output[0] > 0.5) ? 1 : 0;  // Threshold at 0.5
        
        if (prediction == testExpectedOutput[i][0]) {
            correctPredictions++;
        }
        
        Serial.print("Test ");
        Serial.print(i + 1);
        Serial.print(" - Raw Output: ");
        Serial.print(output[0], 4);
        Serial.print(" Predicted: ");
        Serial.print(prediction);
        Serial.print(" Expected: ");
        Serial.println(testExpectedOutput[i][0]);
    }
    
    float accuracy = (correctPredictions / NumberOf(testInputs)) * 100;
    float avgMSE = totalMSE / NumberOf(testInputs);
    
    Serial.println("\n=-[FINAL METRICS]-=");
    Serial.print("Classification Accuracy: ");
    Serial.print(accuracy);
    Serial.println("%");
    Serial.print("Test Set MSE: ");
    Serial.println(avgMSE, 6);
}

void loop() {
    float correctPredictions = 0;
    float totalMSE = 0;
    
    Serial.println("\n=-[TEST RESULTS]-=");
    
    for (unsigned int i = 0; i < NumberOf(testInputs); i++) {
        output = NN->FeedForward(testInputs[i]);
        
        float error = testExpectedOutput[i][0] - output[0];
        totalMSE += error * error;
        
        float prediction = round(output[0]);
        
        if (prediction == testExpectedOutput[i][0]) {
            correctPredictions++;
        }
        
        Serial.print("Test ");
        Serial.print(i + 1);
        Serial.print(" - Raw Output: ");
        Serial.print(output[0], 4);
        Serial.print(" Predicted: ");
        Serial.print(prediction);
        Serial.print(" Expected: ");
        Serial.println(testExpectedOutput[i][0]);
    }
    
    float accuracy = (correctPredictions / NumberOf(testInputs)) * 100;
    float avgMSE = totalMSE / NumberOf(testInputs);
    
    Serial.println("\n=-[FINAL METRICS]-=");
    Serial.print("Classification Accuracy: ");
    Serial.print(accuracy);
    Serial.println("%");
    Serial.print("Test Set MSE: ");
    Serial.println(avgMSE, 6);
    
    while(true); // Stop execution
}