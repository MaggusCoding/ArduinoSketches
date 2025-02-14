#include "Communication.h"

Communication::Communication() : 
    lockService(BLEConfig::SERVICE_UUID),
    // Characteristic for sending weights FROM Arduino TO Python
    weightsReadCharacteristic(BLEConfig::WEIGHTS_READ_CHAR_UUID, BLERead | BLENotify, sizeof(float) * BLEConfig::CHUNK_SIZE),
    // Characteristic for receiving weights FROM Python TO Arduino
    weightsWriteCharacteristic(BLEConfig::WEIGHTS_WRITE_CHAR_UUID, BLEWrite, sizeof(float) * BLEConfig::CHUNK_SIZE),
    controlCharacteristic(BLEConfig::CONTROL_CHAR_UUID, BLERead | BLEWrite, sizeof(uint8_t)),
    labelCharacteristic(BLEConfig::LABEL_CHAR_UUID, BLERead | BLEWrite, sizeof(int8_t)),
    predictionCharacteristic(BLEConfig::PREDICTION_CHAR_UUID, BLERead | BLENotify, sizeof(float) * 3),
    currentBufferPos(0),
    currentSendPos(0)
{
}

bool Communication::begin() {
    Serial.println("Initializing BLE...");
    
    if (!BLE.begin()) {
        Serial.println("Failed to initialize BLE!");
        return false;
    }
    // Add after BLE.begin()
    //BLE.setConnectionInterval(0x0006, 0x0006); // Min and max intervals, 7.5ms (0x0006 * 1.25ms)
    BLE.setLocalName(BLEConfig::DEVICE_NAME);
    BLE.setAdvertisedService(lockService);
    
    lockService.addCharacteristic(weightsReadCharacteristic);
    lockService.addCharacteristic(weightsWriteCharacteristic);
    lockService.addCharacteristic(controlCharacteristic);
    lockService.addCharacteristic(labelCharacteristic);
    lockService.addCharacteristic(predictionCharacteristic);
    BLE.addService(lockService);

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
                currentBufferPos = 0;
                break;
            case Command::START_TRAINING:
                Serial.println("Received START_TRAINING command");
                break;
            case Command::START_CLASSIFICATION:
                Serial.println("Received START_CLASSIFICATION command");
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
        Serial.println("Not connected");
        return false;
    }

    const size_t chunk_size = BLEConfig::CHUNK_SIZE;
    const size_t chunk_bytes = chunk_size * sizeof(float);
    
    while (currentSendPos < length) {
        size_t floatsToSend = min(chunk_size, length - currentSendPos);
        size_t bytesToSend = floatsToSend * sizeof(float);
        
        memcpy(tempBuffer, &weights[currentSendPos], bytesToSend);
        
        if (weightsReadCharacteristic.writeValue(reinterpret_cast<uint8_t*>(tempBuffer), bytesToSend)) {
            currentSendPos += floatsToSend;
            delay(20);
            
            if (currentSendPos >= length) {
                currentSendPos = 0;
                currentCommand = Command::NONE;
                Serial.println("Completed sending all weights");
                return true;
            }
        } else {
            Serial.println("Failed to send weight chunk");
            return false;
        }
    }
    return false;
}

bool Communication::receiveWeights(float* buffer, size_t length) {
    if (!isConnected() || length > NNConfig::MAX_WEIGHTS) {
        Serial.println("Not connected or buffer too large");
        return false;
    }
    
    if (weightsWriteCharacteristic.written()) {
        const size_t max_chunk_size = BLEConfig::CHUNK_SIZE * sizeof(float);
        uint8_t chunk[max_chunk_size];
        int bytesRead = weightsWriteCharacteristic.readValue(chunk, sizeof(chunk));
        
        int numFloats = bytesRead / sizeof(float);
        
        if (currentBufferPos + numFloats > length) {
            Serial.println("Error: Buffer overflow");
            currentBufferPos = 0;
            return false;
        }
        
        memcpy(&buffer[currentBufferPos], chunk, bytesRead);
        currentBufferPos += numFloats;
        
        if (currentBufferPos % 32 == 0) {
            Serial.print("Received weights: ");
            Serial.print(currentBufferPos);
            Serial.print("/");
            Serial.println(length);
        }
        
        if (currentBufferPos >= length) {
            currentBufferPos = 0;
            Serial.println("Weight transfer complete");
            return true;
        }
    }
    return false;
}

bool Communication::sendPrediction(const float* probabilities, size_t length) {
    if (!isConnected() || length != 3) {
        Serial.println("Not connected or invalid prediction length");
        return false;
    }
    
    bool success = predictionCharacteristic.writeValue(probabilities, length * sizeof(float));
    if (success) {
        Serial.println("Sent prediction probabilities");
    } else {
        Serial.println("Failed to send prediction probabilities");
    }
    return success;
}

int8_t Communication::getTrainingLabel() {
    int8_t label = -1;
    labelCharacteristic.readValue(label);
    Serial.print("Received training label: ");
    Serial.println(label);
    return label;
}

void Communication::resetState() {
    currentBufferPos = 0;
    currentSendPos = 0;
    currentCommand = Command::NONE;
    Serial.println("Communication state reset");
}