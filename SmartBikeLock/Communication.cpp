// Communication.cpp
#include "Communication.h"

Communication::Communication() : 
    lockService(serviceUUID),
    weightsCharacteristic(weightsCharUUID, BLERead | BLEWrite | BLENotify, sizeof(float) * 4),
    controlCharacteristic(controlCharUUID, BLERead | BLEWrite, sizeof(uint8_t)),
    currentBufferPos(0)  // Initialize member variable
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

    // Set event handlers
    BLE.setEventHandler(BLEConnected, Communication::onBLEConnected);
    BLE.setEventHandler(BLEDisconnected, Communication::onBLEDisconnected);

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

void Communication::onBLEConnected(BLEDevice central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
}

void Communication::onBLEDisconnected(BLEDevice central) {
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
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
    
    while (currentSendPos < length) {
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
            delay(50);  // Add delay between chunks
            
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
    if (!isConnected() || length > MAX_WEIGHTS) {
        Serial.println("Not connected or buffer too large");
        return false;
    }
    
    if (weightsCharacteristic.written()) {
        uint8_t chunk[16];
        int bytesRead = weightsCharacteristic.readValue(chunk, sizeof(chunk));
        
        int numFloats = bytesRead / 4;
        
        // Validate buffer position
        if (currentBufferPos >= length) {
            Serial.println("Buffer position out of bounds, resetting");
            currentBufferPos = 0;
            return false;
        }
        
        Serial.print("Processing ");
        Serial.print(numFloats);
        Serial.print(" floats. Current position: ");
        Serial.print(currentBufferPos);
        Serial.print("/");
        Serial.println(length);
        
        for(int i = 0; i < numFloats && currentBufferPos < length; i++) {
            float value;
            memcpy(&value, &chunk[i * 4], 4);
            
            // Bounds check
            if (currentBufferPos < MAX_WEIGHTS) {
                tempBuffer[currentBufferPos] = value;
                Serial.print("Stored weight at position ");
                Serial.print(currentBufferPos);
                Serial.print(": ");
                Serial.println(value);
                currentBufferPos++;
                
                if (currentBufferPos == length) {
                    memcpy(buffer, tempBuffer, length * sizeof(float));
                    currentBufferPos = 0;
                    currentCommand = Command::NONE;
                    Serial.println("Received all weights. Resetting state.");
                    return true;
                }
            } else {
                Serial.println("Buffer overflow prevented");
                currentBufferPos = 0;
                return false;
            }
        }
    }
    return false;
}

void Communication::resetState() {
    currentBufferPos = 0;
    currentCommand = Command::NONE;
    Serial.println("Communication state reset");
}
