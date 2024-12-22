// SmartBikeLock.ino
#include "Communication.h"

Communication bleComm;

// Current weights array
float currentWeights[10] = {0.75, -0.23, 0.91, 0.45, -0.82, 0.31, -0.77, 0.44, -0.12, 0.89};
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
        // Check for commands
        if (bleComm.getCurrentCommand() == Command::GET_WEIGHTS) {
            bleComm.sendWeights(currentWeights, 10);
        }
        else if (bleComm.getCurrentCommand() == Command::SET_WEIGHTS) {
            if (bleComm.receiveWeights(receivedWeights, 10)) {
                // Copy new weights to current weights
                memcpy(currentWeights, receivedWeights, sizeof(currentWeights));
                Serial.println("Updated current weights with received weights");
            }
        }
    }
    
    delay(100);
}