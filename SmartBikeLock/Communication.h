// Communication.h
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <ArduinoBLE.h>

class Communication {
public:
    Communication();
    bool begin();
    void update();
    bool isConnected();
    bool receiveWeights(float* buffer, size_t length);

private:
    const char* deviceName = "SmartBikeLock";
    const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
    const char* weightsCharUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
    
    BLEService lockService;
    BLECharacteristic weightsCharacteristic;
    
    // Variables to handle chunked data
    float tempBuffer[10];  // Temporary buffer to store incoming chunks
    size_t currentBufferPos = 0;
};

#endif