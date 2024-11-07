#define NumberOf(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))
#define _2_OPTIMIZE 0B00000000
#define _1_OPTIMIZE 0B00010000

#include <NeuralNetwork.h>

// Global variables
NeuralNetwork *NN;  
float *output;      

const unsigned int layers[] = {11, 40, 30, 1};

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

// Timing variables
unsigned long inferenceTime = 0;  // Pure forward pass (inference)
unsigned long trainingTime = 0;   // Complete training iteration (forward + backward)
unsigned long totalInferenceTime = 0;
unsigned long totalTrainingTime = 0;
unsigned long startTime = 0;

void setup() {
    Serial.begin(9600);
    while(!Serial);
    
    Serial.println("Starting training...");
    
    NN = new NeuralNetwork(layers, NumberOf(layers));
    unsigned int epoch = 0;
    float currentMSE;
    
    do {
        epoch++;
        totalInferenceTime = 0;
        totalTrainingTime = 0;
        
        for (unsigned int j = 0; j < NumberOf(inputs); j++) {
            // Measure pure inference time (forward pass only)
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
        
        // Print timing information
        Serial.print("Epoch ");
        Serial.print(epoch);
        Serial.print(" - MSE: "); 
        Serial.print(currentMSE, 6);
        Serial.print(" | Avg Inference: ");
        Serial.print(totalInferenceTime / NumberOf(inputs));
        Serial.print("µs | Avg Training Iteration: ");
        Serial.print(totalTrainingTime / NumberOf(inputs));
        Serial.println("µs");
        
        if(epoch >= 1000) {
            Serial.println("Maximum epochs reached!");
            break;
        }
    } while(currentMSE > 0.01);

    Serial.println("\n=-[TRAINING COMPLETE]-=");
    
    // Print total training stats
    Serial.println("\n=-[TRAINING STATS]-=");
    Serial.print("Average Inference Time (Forward Pass): ");
    Serial.print(totalInferenceTime / NumberOf(inputs));
    Serial.println("µs");
    Serial.print("Average Training Iteration (Forward + Backward): ");
    Serial.print(totalTrainingTime / NumberOf(inputs));
    Serial.println("µs");
    Serial.print("Implied Backward Pass Time: ");
    Serial.print((totalTrainingTime - totalInferenceTime) / NumberOf(inputs));
    Serial.println("µs");
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