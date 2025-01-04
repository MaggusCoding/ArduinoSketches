// Config.h
#ifndef CONFIG_H
#define CONFIG_H

// Neural Network Configuration
namespace NNConfig {
    constexpr unsigned int NUM_LAYERS = 4;
    constexpr unsigned int LAYERS[NUM_LAYERS] = {11, 20, 10, 1};  // Updated for 3-class classification
    
    constexpr float ERROR_THRESHOLD = 0.01f;
    constexpr unsigned int MAX_EPOCHS = 1000;
    
    constexpr size_t calculateTotalWeights() {
        size_t total = 0;
        for (unsigned int i = 0; i < NUM_LAYERS - 1; i++) {
            total += LAYERS[i] * LAYERS[i + 1];
        }
        return total;
    }
    
    constexpr size_t MAX_WEIGHTS = calculateTotalWeights();

    // Classification labels
    enum class TheftClass {
        NO_THEFT = 0,
        CARRYING_AWAY = 1,
        LOCK_BREACH = 2
    };
}

// Signal Processing Configuration
namespace SignalConfig {
    constexpr unsigned int SAMPLES = 128;
    constexpr unsigned int SAMPLING_FREQ = 100;
    constexpr unsigned int SAMPLING_PERIOD_MS = 1000/SAMPLING_FREQ;
    constexpr unsigned int FEATURE_BINS = 8;
    constexpr unsigned int TOTAL_FEATURES = 11;  // 8 frequency bins + 3 statistical features
    
    // Frequency bands (Hz)
    constexpr float FREQ_BANDS[] = {0, 5, 10, 15, 20, 25, 30, 40, 50};
}

// BLE Communication Configuration
namespace BLEConfig {
    constexpr char DEVICE_NAME[] = "SmartBikeLock";
    constexpr char SERVICE_UUID[] = "19B10000-E8F2-537E-4F6C-D104768A1214";
    constexpr char WEIGHTS_CHAR_UUID[] = "19B10001-E8F2-537E-4F6C-D104768A1214";
    constexpr char CONTROL_CHAR_UUID[] = "19B10002-E8F2-537E-4F6C-D104768A1214";
}

#endif