#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <ArduinoBLE.h>
#include "Config.h"

enum class Command {
    NONE = 0,
    GET_WEIGHTS = 1,
    SET_WEIGHTS = 2
};

class Communication {
public:
    Communication();
    bool begin();
    void update();
    bool isConnected();
    Command getCurrentCommand() { return currentCommand; } 
    bool sendWeights(const float* weights, size_t length);
    bool receiveWeights(float* buffer, size_t length);
    void resetState();

private:
    BLEService lockService;
    BLECharacteristic weightsCharacteristic;
    BLECharacteristic controlCharacteristic;
    
    // Variables for chunked transfer
    size_t currentSendPos = 0;
    Command currentCommand = Command::NONE;
    float tempBuffer[NNConfig::MAX_WEIGHTS];
    size_t currentBufferPos;

    static void onBLEConnected(BLEDevice central);
    static void onBLEDisconnected(BLEDevice central);
};

#endif