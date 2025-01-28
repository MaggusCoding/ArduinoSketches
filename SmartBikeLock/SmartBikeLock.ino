#include "Communication.h"
#include "NeuralNetworkBikeLock.h"
#include "SignalProcessing.h"
#include "TimingBenchmark.h"

#define DEBUG // Uncomment this line if you want debug messages

Communication bleComm;
NeuralNetworkBikeLock NN;
SignalProcessing signalProc;
TimingBenchmark benchmark(NN, signalProc);

void setup() {
    #ifdef DEBUG
    Serial.begin(9600);
    delay(1000);
    Serial.println("Starting setup...");
    #endif
    
    if (!bleComm.begin()) {
        #ifdef DEBUG
        Serial.println("Failed to initialize BLE communication!");
        while (1) {
            Serial.println("BLE init failed");
            delay(1000);
        }
        #endif
    }
    
    #ifdef DEBUG
    Serial.println("BLE initialized");
    #endif
    
    if (!signalProc.begin()) {
        #ifdef DEBUG
        Serial.println("Failed to initialize IMU!");
        while (1) {
            Serial.println("IMU init failed");
            delay(1000);
        }
        #endif
    }
    
    #ifdef DEBUG
    Serial.println("IMU initialized");
    #endif
    
    #ifdef DEBUG
    Serial.println("Initializing Neural Network...");
    #endif
    NN.init(NNConfig::LAYERS, nullptr, NNConfig::NUM_LAYERS);
    
    #ifdef DEBUG
    Serial.println("Neural network initialized!");
    #endif
}

void loop() {
    bleComm.update();

    switch (bleComm.getCurrentCommand()) {
        case Command::START_CLASSIFICATION: {
            #ifdef DEBUG
            Serial.println("Starting classification...");
            #endif
            
            if (signalProc.collectData()) {
                signalProc.processData();
                const float* features = signalProc.getFeatures();
                
                // Perform classification
                float probabilities[3];
                NN. getPredictionProbabilities(features, probabilities);
                
                // Send prediction probabilities
                bleComm.sendPrediction(probabilities, 3);
                
                #ifdef DEBUG
                Serial.println("Classification complete");
                #endif
            }
            
            bleComm.resetState();
            break;
        }
        
        case Command::START_TRAINING: {
            #ifdef DEBUG
            Serial.println("Starting training...");
            #endif
            
            if (signalProc.collectData()) {
                signalProc.processData();
                const float* features = signalProc.getFeatures();
                
                // Get label from BLE characteristic
                int8_t label = bleComm. getTrainingLabel();
                if (label >= 0 && label <= 2) {
                    NN.performLiveTraining(features, label);
                    
                    #ifdef DEBUG
                    Serial.println("Training complete");
                    #endif
                } else {
                    #ifdef DEBUG
                    Serial.println("Invalid label received");
                    #endif
                }
            }
            
            bleComm.resetState();
            break;
        }
        
        case Command::GET_WEIGHTS: {
            size_t numWeights = NN. getTotalWeights();
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
        case Command::START_INFERENCE_BENCHMARK: {
              Serial.println("Starting inference benchmark...");
              benchmark.measureInferenceLatency();
              bleComm.resetState();
              break;
          }

          case Command::START_TRAINING_BENCHMARK: {
              Serial.println("Starting training benchmark...");
              int8_t label = bleComm.getTrainingLabel();
              if (label >= 0 && label <= 2) {
                  benchmark.measureTrainingTime(label);
              }
              bleComm.resetState();
              break;
          }
    }
    
    delay(50);
}
