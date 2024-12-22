#include "Communication.h"

Communication bleComm;

// Array to store all 10 weights
float receivedWeights[10]; 

void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    Serial.println("Starting SmartBikeLock...");
    
    if (!bleComm.begin()) {
        Serial.println("Failed to initialize BLE communication!");
        while (1);
    }
}

void loop() {
    bleComm.update();
    
    if (bleComm.isConnected()) {
        // Check for received weights, passing the full array size
        if (bleComm.receiveWeights(receivedWeights, 10)) {
            // Data will be printed inside receiveWeights when complete
        }
    }
    
    delay(100); // Reduced delay for more responsive updates
}