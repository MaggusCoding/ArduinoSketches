#include "Communication.h"
#include "NeuralNetworkBikeLock.h"
#include "SignalProcessing.h"

Communication bleComm;
NeuralNetworkBikeLock NN;
SignalProcessing signalProc;


void setup() {
    Serial.begin(9600);
    delay(1000);
    Serial.println("Starting setup...");
    
    if (!bleComm.begin()) {
        Serial.println("Failed to initialize BLE communication!");
        while (1) {
            Serial.println("BLE init failed");
            delay(1000);
        }
    }
    Serial.println("BLE initialized");
    
    if (!signalProc.begin()) {
        Serial.println("Failed to initialize IMU!");
        while (1) {
            Serial.println("IMU init failed");
            delay(1000);
        }
    }
    Serial.println("IMU initialized");
    
    Serial.println("Initializing Neural Network...");
    NN.init(NNConfig::LAYERS, nullptr, NNConfig::NUM_LAYERS);
    Serial.println("Neural network initialized!");
}

void loop() {
    bleComm.update();
    
    // Handle BLE commands
    switch (bleComm.getCurrentCommand()) {
        case Command::START_CLASSIFICATION: {
            Serial.println("Starting classification...");
            
            if (signalProc.collectData()) {
                signalProc.processData();
                const float* features = signalProc.getFeatures();
                // Perform classification
                float probabilities[3];
                NN.getPredictionProbabilities(features, probabilities);
                
                // Send prediction probabilities
                bleComm.sendPrediction(probabilities, 3);
                
                Serial.println("Classification complete");
            }
            bleComm.resetState();
            break;
        }
        
        case Command::START_TRAINING: {
            Serial.println("Starting training...");
            
            if (signalProc.collectData()) {
                signalProc.processData();
                const float* features = signalProc.getFeatures();
                // Get label from BLE characteristic
                int8_t label = bleComm.getTrainingLabel();
                if (label >= 0 && label <= 2) {
                    NN.performLiveTraining(features, label);
                    Serial.println("Training complete");
                } else {
                    Serial.println("Invalid label received");
                }
            }
            bleComm.resetState();
            break;
        }
        case Command::GET_WEIGHTS: {
        size_t numWeights = NN.getTotalWeights();
        float* currentWeights = new float[numWeights];
        
        if (NN.getWeights(currentWeights, numWeights)) {
            bleComm.sendWeights(currentWeights, numWeights);
        }
        
        delete[] currentWeights;
        break;
      }
        case Command::SET_WEIGHTS: {
        static float tempWeights[NNConfig::MAX_WEIGHTS];
        if (bleComm.receiveWeights(tempWeights, NNConfig::MAX_WEIGHTS)) {
            NN.updateNetworkWeights(tempWeights, NNConfig::MAX_WEIGHTS);
        }
        break;
      }
    }
    delay(50);
}