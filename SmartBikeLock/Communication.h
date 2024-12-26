// Communication.h
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <ArduinoBLE.h>

enum class Command {
    NONE = 0,
    GET_WEIGHTS = 1,
    SET_WEIGHTS = 2
};

class Communication {
public:
    Communication(); // Initialize in constructor
    bool begin();
    void update();
    bool isConnected();
    Command getCurrentCommand() { return currentCommand; }  // Add this getter
    
    // Functions for sending and receiving weights
    bool sendWeights(const float* weights, size_t length);
    bool receiveWeights(float* buffer, size_t length);
    void resetState();

private:
    const char* deviceName = "SmartBikeLock";
    const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
    const char* weightsCharUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
    const char* controlCharUUID = "19B10002-E8F2-537E-4F6C-D104768A1214";
    
    BLEService lockService;
    BLECharacteristic weightsCharacteristic;
    BLECharacteristic controlCharacteristic;
    
    // Variables for chunked transfer
    size_t currentSendPos = 0;
    Command currentCommand = Command::NONE;
    static const size_t MAX_WEIGHTS = 117;  // Set to your actual max weights
    float tempBuffer[MAX_WEIGHTS];
    size_t currentBufferPos;  // Make sure this is size_t, not int
};

#endif