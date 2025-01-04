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
        
        // Get probabilities for all classes
        float probabilities[3];
        NN.getPredictionProbabilities(features, probabilities);
        
        // Display results
        Serial.println("\nClassification Results:");
        Serial.print("No theft: "); Serial.print(probabilities[0] * 100, 1); Serial.println("%");
        Serial.print("Carrying away: "); Serial.print(probabilities[1] * 100, 1); Serial.println("%");
        Serial.print("Lock breach: "); Serial.print(probabilities[2] * 100, 1); Serial.println("%");
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
        
        Serial.println("\nEnter label (0-2):");
        Serial.println("0: No theft attempt");
        Serial.println("1: Carrying away attempt");
        Serial.println("2: Lock breach attempt");
        
        while(true) {
            if(Serial.available() > 0) {
                char input = Serial.read();
                if(input >= '0' && input <= '2') {
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
    
    randomSeed(analogRead(A0));
    
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
    
    Serial.println("Setup complete!");
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
    
    delay(50);
}