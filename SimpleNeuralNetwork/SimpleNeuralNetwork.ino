#define NumberOf(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))
#define _2_OPTIMIZE 0B00000000
#define _1_OPTIMIZE 0B00010000

#include <NeuralNetwork.h>

// Global variables
NeuralNetwork *NN;  
float *output;      

const unsigned int layers[] = {6, 8, 1};

// Training data
const float inputs[12][6] = {
    {0.1, 0.2, 0.3, 0.1, 0.1, 0.1},  // Normal 
    {0.8, 0.7, 0.9, 0.8, 0.7, 0.9},  // Suspicious
    {0.2, 0.3, 0.2, 0.2, 0.3, 0.2},  // Normal
    {0.9, 0.8, 0.9, 0.7, 0.8, 0.7},  // Suspicious
    {0.1, 0.1, 0.2, 0.3, 0.2, 0.1},  // Normal
    {0.7, 0.9, 0.8, 0.9, 0.7, 0.8},  // Suspicious
    {0.3, 0.2, 0.1, 0.2, 0.1, 0.3},  // Normal
    {0.8, 0.9, 0.7, 0.8, 0.9, 0.7},  // Suspicious
    {0.2, 0.1, 0.3, 0.1, 0.2, 0.2},  // Normal
    {0.9, 0.7, 0.8, 0.9, 0.8, 0.9},  // Suspicious
    {0.1, 0.3, 0.1, 0.2, 0.3, 0.1},  // Normal
    {0.7, 0.8, 0.9, 0.7, 0.9, 0.8}   // Suspicious
};

const float expectedOutput[12][1] = {
    {0}, {1}, {0}, {1}, {0}, {1}, 
    {0}, {1}, {0}, {1}, {0}, {1}
};

// Test data
const float testInputs[6][6] = {
    {0.15, 0.25, 0.35, 0.15, 0.15, 0.15},  // Should be normal
    {0.85, 0.75, 0.85, 0.75, 0.75, 0.85},  // Should be suspicious
    {0.25, 0.35, 0.25, 0.25, 0.35, 0.25},  // Should be normal
    {0.75, 0.85, 0.75, 0.85, 0.75, 0.85},  // Should be suspicious
    {0.15, 0.15, 0.25, 0.35, 0.25, 0.15},  // Should be normal
    {0.85, 0.95, 0.85, 0.75, 0.85, 0.75}   // Should be suspicious
};

const float testExpectedOutput[6][1] = {
    {0}, {1}, {0}, {1}, {0}, {1}
};

void setup() {
    Serial.begin(9600);
    while(!Serial);
    
    Serial.println("Starting training...");
    
    NN = new NeuralNetwork(layers, NumberOf(layers));
    unsigned int epoch = 0;
    float currentMSE;
    
    do {
        epoch++;
        for (unsigned int j = 0; j < NumberOf(inputs); j++) {
            NN->FeedForward(inputs[j]);      
            NN->BackProp(expectedOutput[j]); 
        }
        
        currentMSE = NN->getMeanSqrdError(NumberOf(inputs));
        Serial.print("Epoch ");
        Serial.print(epoch);
        Serial.print(" - MSE: "); 
        Serial.println(currentMSE, 6);
        
        if(epoch >= 1000) {
            Serial.println("Maximum epochs reached!");
            break;
        }
    } while(currentMSE > 0.01);

    Serial.println("\n=-[TRAINING COMPLETE]-=");
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