#include "Communication.h"
#include "NeuralNetworkBikeLock.h"
#include "SignalProcessing.h"

Communication bleComm;
NeuralNetworkBikeLock NN;
SignalProcessing signalProc;

void performClassification() {
    Serial.println("Starting classification...");
    
    if (signalProc.collectData()) {
        Serial.println("Data collected, processing...");
        signalProc.processData();
        const float* features = signalProc.getFeatures();
        Serial.println("Features extracted");
        
        float probabilities[1];  // For binary classification
        NN.getPredictionProbabilities(features, probabilities);
        float prediction = probabilities[0];
        
        Serial.println("\nClassification Results:");
        Serial.print("Raw output: "); Serial.println(prediction, 4);
        Serial.print("Classification: ");
        if (prediction > 0.5) {
            Serial.println("SUSPICIOUS");
            Serial.print("Confidence: ");
            Serial.print((prediction - 0.5) * 200, 1);
        } else {
            Serial.println("NORMAL");
            Serial.print("Confidence: ");
            Serial.print((0.5 - prediction) * 200, 1);
        }
        Serial.println("%");
    }
    
    Serial.println("\nSend 'r' to record and train, or 'c' for classification");
}

void recordAndTrain() {
    Serial.println("Recording data...");
    
    if (signalProc.collectData()) {
        Serial.println("Data collected, processing...");
        signalProc.processData();
        const float* features = signalProc.getFeatures();
        Serial.println("Features extracted");
        
        // Display feature values for verification
        Serial.println("\nFeature values:");
        for(int i = 0; i < SignalConfig::FEATURE_BINS; i++) {
            Serial.print("Freq ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(features[i], 6);
        }
        
        Serial.println("\nEnter label (0-1):");
        Serial.println("0: Normal behavior");
        Serial.println("1: Suspicious behavior");
        
        while(true) {
            if(Serial.available() > 0) {
                char input = Serial.read();
                if(input == '0' || input == '1') {
                    int label = input - '0';
                    Serial.print("Training with label: ");
                    Serial.println(label);
                    
                    NN.performLiveTraining(features, label);
                    break;
                }
            }
            delay(10);
        }
    }
    
    Serial.println("Training complete");
    Serial.println("\nSend 'r' to record and train, or 'c' for classification");
}

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
    Serial.println("Send 'r' to record and train, or 'c' for classification");
}

void loop() {
    bleComm.update();
    
    if (Serial.available() > 0) {
        char input = Serial.read();
        Serial.print("Received command: ");
        Serial.println(input);
        
        if (input == 'c' || input == 'C') {
            performClassification();
        }
        else if (input == 'r' || input == 'R') {
            recordAndTrain();
        }
    }
    
    // Handle BLE communication
    if (bleComm.getCurrentCommand() == Command::GET_WEIGHTS) {
        size_t numWeights = NN.getTotalWeights();
        float* currentWeights = new float[numWeights];
        
        if (NN.getWeights(currentWeights, numWeights)) {
            bleComm.sendWeights(currentWeights, numWeights);
        }
        
        delete[] currentWeights;
    }
    else if (bleComm.getCurrentCommand() == Command::SET_WEIGHTS) {
        static float tempWeights[NNConfig::MAX_WEIGHTS];
        if (bleComm.receiveWeights(tempWeights, NNConfig::MAX_WEIGHTS)) {
            NN.updateNetworkWeights(tempWeights, NNConfig::MAX_WEIGHTS);
        }
    }
    
    delay(50);
}