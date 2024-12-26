#define _2_OPTIMIZE 0B01000000
#define _1_OPTIMIZE 0B00010000
#include "NeuralNetworkBikeLock.h"
#include <NeuralNetwork.h>

NeuralNetworkBikeLock::NeuralNetworkBikeLock() : nn(nullptr), isInitialized(false) {
}

void NeuralNetworkBikeLock::init(const unsigned int* layer_, float* default_Weights, const unsigned int& NumberOflayers) {
    Serial.println("Starting NN initialization...");
    if (!isInitialized) {
        numLayers = NumberOflayers;
        Serial.print("Number of layers: ");
        Serial.println(numLayers);
        
        layers = new unsigned int[numLayers];
        memcpy(layers, layer_, numLayers * sizeof(unsigned int));
        
        Serial.println("Layer configuration:");
        for(unsigned int i = 0; i < numLayers; i++) {
            Serial.print(layers[i]);
            if(i < numLayers - 1) Serial.print(" -> ");
        }
        Serial.println();
        
        nn = new NeuralNetwork(layer_, default_Weights, NumberOflayers);
        isInitialized = true;
        Serial.println("Neural Network initialized successfully");
    } else {
        Serial.println("Neural Network already initialized");
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


bool NeuralNetworkBikeLock::processInput(const float* input, size_t length) {
    if (!isInitialized) return false;
    
    // Process input through neural network
    float* output = nn->FeedForward(input);
    
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