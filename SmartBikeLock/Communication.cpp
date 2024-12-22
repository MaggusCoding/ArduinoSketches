// Communication.cpp
#include "Communication.h"

Communication::Communication() : 
    lockService(serviceUUID),
    weightsCharacteristic(weightsCharUUID, BLEWrite | BLERead | BLENotify, sizeof(float) * 4) // 4 floats per chunk
{
}

bool Communication::begin() {
    Serial.println("Initializing BLE...");
    
    if (!BLE.begin()) {
        Serial.println("Failed to initialize BLE!");
        return false;
    }

    BLE.setLocalName(deviceName);
    BLE.setAdvertisedService(lockService);
    
    lockService.addCharacteristic(weightsCharacteristic);
    BLE.addService(lockService);

    BLE.advertise();
    Serial.println("BLE service started");
    Serial.print("Device MAC: ");
    Serial.println(BLE.address());
    return true;
}

void Communication::update() {
    BLE.poll();
}

bool Communication::isConnected() {
    return BLE.connected();
}

bool Communication::receiveWeights(float* buffer, size_t length) {
    if (!isConnected()) {
        Serial.println("Cannot receive: Not connected");
        return false;
    }
    
    if (weightsCharacteristic.written()) {
        uint8_t chunk[16];  // 4 floats * 4 bytes each = 16 bytes
        int bytesRead = weightsCharacteristic.readValue(chunk, sizeof(chunk));
        
        // Convert bytes to floats (4 floats per chunk)
        int numFloats = bytesRead / 4;
        for(int i = 0; i < numFloats && currentBufferPos < length; i++) {
            float value;
            memcpy(&value, &chunk[i * 4], 4);
            tempBuffer[currentBufferPos++] = value;
            
            // If we've received all floats, copy to main buffer and reset
            if (currentBufferPos == length) {
                memcpy(buffer, tempBuffer, length * sizeof(float));
                currentBufferPos = 0;
                
                // Print all received weights
                Serial.println("\nReceived all weights:");
                Serial.println("--------------------");
                for(size_t i = 0; i < length; i++) {
                    Serial.print("Weight[");
                    Serial.print(i);
                    Serial.print("]: ");
                    Serial.println(buffer[i], 6);
                }
                Serial.println("--------------------");
                return true;
            }
        }
    }
    return false;
}