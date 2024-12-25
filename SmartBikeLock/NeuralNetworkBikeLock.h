#ifndef NEURAL_NETWORK_BIKE_LOCK_H
#define NEURAL_NETWORK_BIKE_LOCK_H

#include <stddef.h> // for size_t


// Forward declaration
class NeuralNetwork;

class NeuralNetworkBikeLock {
public:
    NeuralNetworkBikeLock();
    void init(const unsigned int* layer_, float* weights, const unsigned int& NumberOflayers);
    bool processInput(const float* input, size_t length);
    bool updateWeights(const float* newWeights, size_t length);
    bool getWeights(float* buffer, size_t length);
    size_t getTotalWeights();
    
private:
    NeuralNetwork* nn;  // Change to pointer
    unsigned int* layers;
    unsigned int numLayers;
    bool isInitialized;
};

#endif
