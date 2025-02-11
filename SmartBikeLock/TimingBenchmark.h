#ifndef TIMING_BENCHMARK_H
#define TIMING_BENCHMARK_H

#include "NeuralNetworkBikeLock.h"
#include "SignalProcessing.h"

class TimingBenchmark {
private:
    struct TimingMetrics {
        unsigned long dataCollectionTime;
        unsigned long featureExtractionTime;
        unsigned long inferenceTime;
        unsigned long trainingTime;
        unsigned long totalTime;
    };

    TimingMetrics metrics;
    NeuralNetworkBikeLock& nn;
    SignalProcessing& signalProc;
    
    static const int NUM_ITERATIONS = 10;
    
public:
    TimingBenchmark(NeuralNetworkBikeLock& nn_, SignalProcessing& signalProc_) 
        : nn(nn_), signalProc(signalProc_) {
        resetMetrics();
    }
    
    void resetMetrics() {
        metrics = {0, 0, 0, 0, 0};
    }
    
    void measureInferenceLatency() {
        Serial.println("\nMeasuring inference latency...");
        resetMetrics();
        
        for(int i = 0; i < NUM_ITERATIONS; i++) {
            unsigned long startTotal = micros();
            
            // Measure data collection
            unsigned long start = micros();
            signalProc.collectData();
            unsigned long dataCollectionTime = micros() - start;
            
            // Measure feature extraction
            start = micros();
            signalProc.processData();
            const float* features = signalProc.getFeatures();
            unsigned long featureTime = micros() - start;
            
            // Measure inference
            start = micros();
            float probabilities[3];
            nn.getPredictionProbabilities(features, probabilities);
            unsigned long inferenceTime = micros() - start;
            
            // Update metrics
            metrics.dataCollectionTime += dataCollectionTime;
            metrics.featureExtractionTime += featureTime;
            metrics.inferenceTime += inferenceTime;
            metrics.totalTime += (micros() - startTotal);
            
            if((i + 1) % 10 == 0) {
                Serial.print("Completed ");
                Serial.print(i + 1);
                Serial.println(" iterations");
            }
        }
        
        // Calculate averages
        metrics.dataCollectionTime /= NUM_ITERATIONS;
        metrics.featureExtractionTime /= NUM_ITERATIONS;
        metrics.inferenceTime /= NUM_ITERATIONS;
        metrics.totalTime /= NUM_ITERATIONS;
        
        printInferenceMetrics();
    }
    
    void measureTrainingTime(int label) {
        Serial.println("\nMeasuring training time...");
        resetMetrics();
        
        for(int i = 0; i < NUM_ITERATIONS; i++) {
            unsigned long startTotal = micros();
            
            // Measure data collection
            unsigned long start = micros();
            signalProc.collectData();
            unsigned long dataCollectionTime = micros() - start;
            
            // Measure feature extraction
            start = micros();
            signalProc.processData();
            const float* features = signalProc.getFeatures();
            unsigned long featureTime = micros() - start;
            
            // Measure training
            start = micros();
            nn.performLiveTraining(features, label);
            unsigned long trainingTime = micros() - start;
            
            // Update metrics
            metrics.dataCollectionTime += dataCollectionTime;
            metrics.featureExtractionTime += featureTime;
            metrics.trainingTime += trainingTime;
            metrics.totalTime += (micros() - startTotal);
            
            if((i + 1) % 10 == 0) {
                Serial.print("Completed ");
                Serial.print(i + 1);
                Serial.println(" iterations");
            }
        }
        
        // Calculate averages
        metrics.dataCollectionTime /= NUM_ITERATIONS;
        metrics.featureExtractionTime /= NUM_ITERATIONS;
        metrics.trainingTime /= NUM_ITERATIONS;
        metrics.totalTime /= NUM_ITERATIONS;
        
        printTrainingMetrics();
    }
    
private:
    void printInferenceMetrics() {
        Serial.println("\n=== Inference Timing Results ===");
        Serial.println("Average times over 5 iterations:");
        Serial.print("Data Collection: "); 
        Serial.print(metrics.dataCollectionTime);
        Serial.println(" microseconds");
        
        Serial.print("Feature Extraction: ");
        Serial.print(metrics.featureExtractionTime);
        Serial.println(" microseconds");
        
        Serial.print("Inference: ");
        Serial.print(metrics.inferenceTime);
        Serial.println(" microseconds");
        
        Serial.print("Total Latency: ");
        Serial.print(metrics.totalTime);
        Serial.println(" microseconds");
        
        // Convert to milliseconds for easier reading
        float totalMs = metrics.totalTime / 1000.0;
        Serial.print("Total Latency: ");
        Serial.print(totalMs);
        Serial.println(" milliseconds");
    }
    
    void printTrainingMetrics() {
        Serial.println("\n=== Training Timing Results ===");
        Serial.println("Average times over 5 iterations:");
        Serial.print("Data Collection: ");
        Serial.print(metrics.dataCollectionTime);
        Serial.println(" microseconds");
        
        Serial.print("Feature Extraction: ");
        Serial.print(metrics.featureExtractionTime);
        Serial.println(" microseconds");
        
        Serial.print("Training: ");
        Serial.print(metrics.trainingTime);
        Serial.println(" microseconds");
        
        Serial.print("Total Time: ");
        Serial.print(metrics.totalTime);
        Serial.println(" microseconds");
        
        // Convert to milliseconds for easier reading
        float totalMs = metrics.totalTime / 1000.0;
        Serial.print("Total Time: ");
        Serial.print(totalMs);
        Serial.println(" milliseconds");
    }
};

#endif
