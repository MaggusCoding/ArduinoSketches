#ifndef NEURAL_NETWORK_BIKE_LOCK_H
#define NEURAL_NETWORK_BIKE_LOCK_H

#include <stddef.h>
#include "Config.h"

class NeuralNetwork;

class NeuralNetworkBikeLock {
public:
    NeuralNetworkBikeLock();
    void init(const unsigned int* layer_, float* weights, const unsigned int& NumberOflayers);
    
    // Modified training method to accept label
    void performLiveTraining(const float* features, int label);
    float getMeanSquaredError(size_t numSamples);
    
    // Inference methods
    NNConfig::TheftClass performInference(const float* features);
    void getPredictionProbabilities(const float* features, float* probabilities);
    
    // Weight management
    bool getWeights(float* buffer, size_t length);
    size_t getTotalWeights();
    bool updateNetworkWeights(const float* newWeights, size_t length);
    
private:
    NeuralNetwork* nn;
    unsigned int* layers;
    unsigned int numLayers;
    bool isInitialized;
};

#endif