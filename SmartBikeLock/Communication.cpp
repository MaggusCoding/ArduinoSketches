// Communication.cpp
#include "Communication.h"

Communication::Communication() : 
    lockService(serviceUUID),
    weightsCharacteristic(weightsCharUUID, BLERead | BLEWrite | BLENotify, sizeof(float) * 4),
    controlCharacteristic(controlCharUUID, BLERead | BLEWrite, sizeof(uint8_t))
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
    lockService.addCharacteristic(controlCharacteristic);
    BLE.addService(lockService);

    BLE.advertise();
    Serial.println("BLE service started");
    Serial.print("Device MAC: ");
    Serial.println(BLE.address());
    return true;
}

void Communication::update() {
    BLE.poll();
    
    // Check for control commands
    if (controlCharacteristic.written()) {
        uint8_t command;
        controlCharacteristic.readValue(command);
        currentCommand = static_cast<Command>(command);
        
        switch(currentCommand) {
            case Command::GET_WEIGHTS:
                Serial.println("Received GET_WEIGHTS command");
                break;
            case Command::SET_WEIGHTS:
                Serial.println("Received SET_WEIGHTS command");
                currentBufferPos = 0; // Reset buffer for receiving
                break;
            default:
                Serial.println("Unknown command received");
                break;
        }
    }
}

bool Communication::isConnected() {
    return BLE.connected();
}

bool Communication::sendWeights(const float* weights, size_t length) {
    if (!isConnected()) {
        return false;
    }

    // Send 4 floats at a time
    const size_t chunk_size = 4;
    uint8_t chunk[16]; // 4 floats * 4 bytes
    
    if (currentSendPos < length) {
        size_t floatsToSend = min(chunk_size, length - currentSendPos);
        size_t bytesToSend = floatsToSend * sizeof(float);
        
        memcpy(chunk, &weights[currentSendPos], bytesToSend);
        
        if (weightsCharacteristic.writeValue(chunk, bytesToSend)) {
            Serial.print("Sent weights: ");
            for (size_t i = 0; i < floatsToSend; i++) {
                Serial.print(weights[currentSendPos + i], 6);
                Serial.print(" ");
            }
            Serial.println();
            
            currentSendPos += floatsToSend;
            
            if (currentSendPos >= length) {
                currentSendPos = 0;
                currentCommand = Command::NONE;
                return true;
            }
        }
    }
    
    return false;
}

bool Communication::receiveWeights(float* buffer, size_t length) {
    if (!isConnected()) {
        return false;
    }
    
    if (weightsCharacteristic.written()) {
        uint8_t chunk[16];
        int bytesRead = weightsCharacteristic.readValue(chunk, sizeof(chunk));
        
        int numFloats = bytesRead / 4;
        for(int i = 0; i < numFloats && currentBufferPos < length; i++) {
            float value;
            memcpy(&value, &chunk[i * 4], 4);
            tempBuffer[currentBufferPos++] = value;
            
            if (currentBufferPos == length) {
                memcpy(buffer, tempBuffer, length * sizeof(float));
                currentBufferPos = 0;
                currentCommand = Command::NONE;
                
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