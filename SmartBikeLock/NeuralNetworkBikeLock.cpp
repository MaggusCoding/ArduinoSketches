#define _2_OPTIMIZE 0B01000000
#include "NeuralNetworkBikeLock.h"
#include <NeuralNetwork.h>

NeuralNetworkBikeLock::NeuralNetworkBikeLock() : nn(nullptr), isInitialized(false) {
}

void NeuralNetworkBikeLock::init(const unsigned int* layer_, float* default_Weights, const unsigned int& NumberOflayers) {
    if (!isInitialized) {
        numLayers = NumberOflayers;
        layers = new unsigned int[numLayers];
        memcpy(layers, layer_, numLayers * sizeof(unsigned int));
        
        
        nn = new NeuralNetwork(layer_, default_Weights, NumberOflayers);
        isInitialized = true;
    }
}

bool NeuralNetworkBikeLock::processInput(const float* input, size_t length) {
    if (!isInitialized) return false;
    
    // Process input through neural network
    float* output = nn -> FeedForward(input);
    
    // Handle the output
    // Add your logic here
    
    return true;
}

bool NeuralNetworkBikeLock::updateWeights(const float* newWeights, size_t length) {
    if (!isInitialized) return false;
    
    // Add logic to update weights
    // This will depend on how the underlying library handles weight updates
    
    return true;
}

float* NeuralNetworkBikeLock::getWeights(size_t& length) {
    if (!isInitialized) {
        length = 0;
        return nullptr;
    }
    
    // Add logic to get current weights
    // This will depend on how the underlying library stores weights
    
    return nullptr;
}