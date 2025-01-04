#define _2_OPTIMIZE 0B01000000
#define _1_OPTIMIZE 0B00010000
#include "NeuralNetworkBikeLock.h"
#include <NeuralNetwork.h>

NeuralNetworkBikeLock::NeuralNetworkBikeLock() : nn(nullptr), isInitialized(false) {
}

void NeuralNetworkBikeLock::init(const unsigned int* layer_, float* weights, const unsigned int& NumberOflayers) {
    Serial.println("Starting NN initialization...");
    if (!isInitialized) {
        numLayers = NumberOflayers;
        
        layers = new unsigned int[numLayers];
        memcpy(layers, layer_, numLayers * sizeof(unsigned int));
        
        // Calculate total weights needed
        size_t totalWeights = getTotalWeights();
        Serial.print("Total weights to initialize: ");
        Serial.println(totalWeights);
        
        // If no weights provided, create random weights
        if (weights == nullptr) {
            nn = new NeuralNetwork(layer_, NumberOflayers);
        } else {
            nn = new NeuralNetwork(layer_, weights, NumberOflayers);
        }
        
        isInitialized = true;
        Serial.println("Neural Network initialized successfully");
        
        // Print layer configuration
        Serial.println("Layer configuration:");
        for(unsigned int i = 0; i < numLayers; i++) {
            Serial.print(layers[i]);
            if(i < numLayers - 1) Serial.print(" -> ");
        }
        Serial.println();
        
        // Verify initialization with a test forward pass
        float testInput[SignalConfig::TOTAL_FEATURES] = {0};
        float* testOutput = nn->FeedForward(testInput);
        Serial.println("Test forward pass results:");
        for(unsigned int i = 0; i < layers[numLayers-1]; i++) {
            Serial.print(testOutput[i], 6);
            Serial.print(" ");
        }
        Serial.println();
    } else {
        Serial.println("Neural Network already initialized");
    }
}

void NeuralNetworkBikeLock::performLiveTraining(const float* features, int label) {
    if (!isInitialized || label < 0 || label > 1) return;  // Changed to check for binary labels
    
    Serial.println("Starting training process...");
    
    // Create binary label (0 or 1)
    float expectedOutput[1] = {static_cast<float>(label)};
    
    Serial.println("Created binary label");
    Serial.print("Target: ");
    Serial.println(expectedOutput[0]);
    
    // First perform forward pass to get current prediction
    Serial.println("Performing forward pass...");
    nn->FeedForward(features);
    nn->BackProp(expectedOutput);  // Pass the array directly
    Serial.println("Backpropagation completed");
    
    // Get updated predictions
    Serial.println("Getting updated predictions...");
    float* output = nn->FeedForward(features);
    
    // Display training results
    Serial.println("\nPrediction after training:");
    Serial.print("Output: "); Serial.println(output[0], 4);
    
    Serial.println("Training process completed");
}

NNConfig::TheftClass NeuralNetworkBikeLock::performInference(const float* features) {
    if (!isInitialized) return NNConfig::TheftClass::NO_THEFT;
    
    float* output = nn->FeedForward(features);
    
    // Find the highest probability class
    float maxProb = output[0];
    int maxIdx = 0;
    
    for(int i = 1; i < 3; i++) {
        if(output[i] > maxProb) {
            maxProb = output[i];
            maxIdx = i;
        }
    }
    
    return static_cast<NNConfig::TheftClass>(maxIdx);
}

// Get prediction probabilities for all classes
void NeuralNetworkBikeLock::getPredictionProbabilities(const float* features, float* probabilities) {
    if (!isInitialized) return;
    
    float* output = nn->FeedForward(features);
    
    // Copy probabilities
    for(int i = 0; i < 3; i++) {
        probabilities[i] = output[i];
    }
}

bool NeuralNetworkBikeLock::getWeights(float* buffer, size_t length) {
    if (!isInitialized || !buffer) return false;
    
    size_t weightIndex = 0;
    
    for (unsigned int i = 0; i < nn->numberOflayers; i++) {
        unsigned int numInputs = nn->layers[i]._numberOfInputs;
        unsigned int numOutputs = nn->layers[i]._numberOfOutputs;
        
        unsigned int layerWeights = numInputs * numOutputs;
        
        if (weightIndex + layerWeights > length) {
            Serial.println("Error: Buffer too small for weights");
            return false;
        }
        
        #if defined(REDUCE_RAM_WEIGHTS_LVL2)
            memcpy(&buffer[weightIndex], 
                   nn->weights + weightIndex, 
                   layerWeights * sizeof(float));
            weightIndex += layerWeights;  // Only increment once for REDUCE_RAM_WEIGHTS_LVL2
        #else
            for (unsigned int out = 0; out < numOutputs; out++) {
                for (unsigned int in = 0; in < numInputs; in++) {
                    buffer[weightIndex++] = nn->layers[i].weights[out][in];
                }
            }
        #endif
    }
    
    return true;
}


bool NeuralNetworkBikeLock::updateNetworkWeights(const float* newWeights, size_t length) {
    if (!isInitialized || !newWeights) {
        Serial.println("Cannot update weights: Network not initialized or invalid weights");
        return false;
    }
    
    // Verify length matches expected total weights
    size_t expectedWeights = getTotalWeights();
    if (length != expectedWeights) {
        Serial.print("Weight count mismatch. Expected: ");
        Serial.print(expectedWeights);
        Serial.print(" Got: ");
        Serial.println(length);
        return false;
    }
    
    // Update weights in the network
    #if defined(REDUCE_RAM_WEIGHTS_LVL2)
        memcpy(nn->weights, newWeights, length * sizeof(float));
    #else
        size_t weightIndex = 0;
        for (unsigned int i = 0; i < nn->numberOflayers; i++) {
            for (unsigned int out = 0; out < nn->layers[i]._numberOfOutputs; out++) {
                for (unsigned int in = 0; in < nn->layers[i]._numberOfInputs; in++) {
                    nn->layers[i].weights[out][in] = newWeights[weightIndex++];
                }
            }
        }
    #endif
    
    Serial.println("Network weights updated successfully");
    return true;
}

size_t NeuralNetworkBikeLock::getTotalWeights() {
    if (!isInitialized) {
        Serial.println("Network not initialized in getTotalWeights");
        return 0;
    }
    
    size_t total = 0;
    for (unsigned int i = 0; i < nn->numberOflayers; i++) {
        unsigned int layerWeights = nn->layers[i]._numberOfInputs * nn->layers[i]._numberOfOutputs;
        total += layerWeights;
        
        Serial.print("Layer ");
        Serial.print(i);
        Serial.print(" weights: ");
        Serial.print(nn->layers[i]._numberOfInputs);
        Serial.print(" x ");
        Serial.print(nn->layers[i]._numberOfOutputs);
        Serial.print(" = ");
        Serial.println(layerWeights);
    }
    
    Serial.print("Total weights needed: ");
    Serial.println(total);
    return total;
}