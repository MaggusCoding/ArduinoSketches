#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>

// Constants for data collection and FFT
const int SAMPLES = 256;              // Power of 2 for FFT
const int SAMPLING_FREQUENCY = 100;   // 100 Hz sampling rate
const int SAMPLING_PERIOD_US = 1000000 / SAMPLING_FREQUENCY;
const float COLLECTION_TIME_SEC = float(SAMPLES) / SAMPLING_FREQUENCY;

// Maximum number of datasets to store in PROGMEM
const int MAX_DATASETS = 25;
const int FEATURES_PER_SAMPLE = 6;  // ax, ay, az, gx, gy, gz

// Structure to hold a labeled dataset
struct Dataset {
  float data[SAMPLES][FEATURES_PER_SAMPLE];
  uint8_t label;  // 0: no theft, 1: theft attempt
};

// Arrays to temporarily store sensor data
float ax[SAMPLES], ay[SAMPLES], az[SAMPLES];
float gx[SAMPLES], gy[SAMPLES], gz[SAMPLES];

// Counter for stored datasets
int datasetCount = 0;


// Datasets stored in PROGMEM
Dataset storedDatasets[MAX_DATASETS] PROGMEM;

// BLE UUIDs
BLEService motionService("fbb9d115-a253-4636-9f9c-9be38930a78a");
// Control characteristic for start/stop (1 byte: 0 = stop, 1 = start)
BLECharacteristic controlChar("19b10001-e8f2-537e-4f6c-d104768a1214", BLEWrite, 1);
// Label characteristic (1 byte: 0 = normal, 1 = theft)
BLECharacteristic labelChar("19b10002-e8f2-537e-4f6c-d104768a1214", BLEWrite, 1);
// Status characteristic (1 byte: 0 = idle, 1 = collecting, 2 = ready for label)
BLECharacteristic statusChar("19b10003-e8f2-537e-4f6c-d104768a1214", BLERead | BLENotify, 1);

// Status codes
const uint8_t STATUS_IDLE = 0;
const uint8_t STATUS_COLLECTING = 1;
const uint8_t STATUS_READY_FOR_LABEL = 2;

// Flag to track if we're currently collecting data
volatile bool isCollecting = false;

void setup() {
    Serial.begin(115200);
    if (Serial) {
        while (!Serial);
    }

    if (!BLE.begin()) {
        if (Serial) {
            Serial.println("Starting BLE failed!");
        }
        while (1);
    }

    // Setup BLE
    BLE.setLocalName("BikeGuard");
    BLE.setAdvertisedService(motionService);
    
    // Add characteristics to the service
    motionService.addCharacteristic(controlChar);
    motionService.addCharacteristic(labelChar);
    motionService.addCharacteristic(statusChar);
    
    // Add the service
    BLE.addService(motionService);

    // Set initial characteristic values
    controlChar.writeValue((uint8_t)STATUS_IDLE);
    labelChar.writeValue((uint8_t)0);
    statusChar.writeValue((uint8_t)STATUS_IDLE);

    // Set up characteristic event handlers
    controlChar.setEventHandler(BLEWritten, onControlWritten);
    labelChar.setEventHandler(BLEWritten, onLabelWritten);

    if (!IMU.begin()) {
        if (Serial) {
            Serial.println("Failed to initialize IMU!");
        }
        while (1);
    }

    // Start advertising
    BLE.advertise();

    if (Serial) {
        Serial.println("Bluetooth device active, waiting for connections...");
        printMenu();
    }
}

void loop() {
    BLE.poll();

    // Handle Serial commands if available
    if (Serial.available()) {
        char command = Serial.read();
        handleSerialCommand(command);
    }

    // If connected to central device and ready to collect
    BLEDevice central = BLE.central();
    if (central && central.connected() && isCollecting) {
        collectAndLabelViaBLE();
        isCollecting = false;
        controlChar.writeValue((uint8_t)0); // Reset control flag
        statusChar.writeValue((uint8_t)STATUS_IDLE);
    }
}

void onControlWritten(BLEDevice central, BLECharacteristic characteristic) {
    uint8_t value = *characteristic.value();
    if (value == 1) {
        isCollecting = true;
        statusChar.writeValue((uint8_t)STATUS_COLLECTING);
        if (Serial) {
            Serial.println("Starting data collection via BLE...");
        }
    }
}

void onLabelWritten(BLEDevice central, BLECharacteristic characteristic) {
    // This will be handled within collectAndLabelViaBLE
}

void collectAndLabelViaBLE() {
    if (datasetCount >= MAX_DATASETS) {
        if (Serial) {
            Serial.println("Error: Maximum number of datasets reached!");
        }
        statusChar.writeValue((uint8_t)STATUS_IDLE);
        return;
    }

    // Collect the data
    collectSamples();

    // Notify that we're ready for label
    statusChar.writeValue((uint8_t)STATUS_READY_FOR_LABEL);
    
    if (Serial) {
        Serial.println("Waiting for label via BLE...");
    }

    // Wait up to 10 seconds for label
    unsigned long startWait = millis();
    while (millis() - startWait < 10000) {
        BLE.poll();
        if (labelChar.written()) {
            uint8_t label = *labelChar.value();
            if (label <= 1) {  // Valid label
                // Create the dataset in a new scope so it gets cleared from stack
                {
                    Dataset newDataset;  // This will be created on the stack
                    newDataset.label = label;
                    
                    // Copy data to dataset structure
                    for (int i = 0; i < SAMPLES; i++) {
                        newDataset.data[i][0] = ax[i];
                        newDataset.data[i][1] = ay[i];
                        newDataset.data[i][2] = az[i];
                        newDataset.data[i][3] = gx[i];
                        newDataset.data[i][4] = gy[i];
                        newDataset.data[i][5] = gz[i];
                    }

                    // Store in PROGMEM
                    memcpy_P(&storedDatasets[datasetCount], &newDataset, sizeof(Dataset));
                }  // newDataset goes out of scope here and its memory is freed
                
                datasetCount++;

                if (Serial) {
                    Serial.print("Dataset stored with label: ");
                    Serial.println(label);
                }
                
                statusChar.writeValue((uint8_t)STATUS_IDLE);
                return;
            }
        }
        delay(10);  // Small delay to prevent tight polling
    }

    if (Serial) {
        Serial.println("Label timeout - data discarded");
    }
    statusChar.writeValue((uint8_t)STATUS_IDLE);
}

void collectSamples() {
    // Clear arrays
    memset(ax, 0, sizeof(ax));
    memset(ay, 0, sizeof(ay));
    memset(az, 0, sizeof(az));
    memset(gx, 0, sizeof(gx));
    memset(gy, 0, sizeof(gy));
    memset(gz, 0, sizeof(gz));
    
    unsigned long startTime = millis();
    
    for(int i = 0; i < SAMPLES; i++) {
        unsigned long startMicros = micros();
        
        float x, y, z;
        
        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(x, y, z);
            ax[i] = x;
            ay[i] = y;
            az[i] = z;
        }
        
        if (IMU.gyroscopeAvailable()) {
            IMU.readGyroscope(x, y, z);
            gx[i] = x;
            gy[i] = y;
            gz[i] = z;
        }
        
        while(micros() - startMicros < SAMPLING_PERIOD_US);
    }
    
    if (Serial) {
        unsigned long actualCollectionTime = millis() - startTime;
        Serial.print("Collection completed in ");
        Serial.print(actualCollectionTime);
        Serial.println(" ms");
    }
}

void handleSerialCommand(char command) {
    switch (command) {
        case 'v':
        case 'V':
            viewStoredDatasets();
            break;
        case 's':
       /* case 'S':
            streamDatasets();
            break;*/
        case 'h':
        case 'H':
            printMenu();
            break;
        default:
            break;
    }
}

void printMenu() {
    Serial.println("\n=== Motion Data Collection Menu ===");
    Serial.println("V: View stored datasets");
    Serial.println("S: Stream datasets to computer");
    Serial.println("H: Show this menu");
    Serial.print("Stored datasets: ");
    Serial.print(datasetCount);
    Serial.print("/");
    Serial.println(MAX_DATASETS);
}

void streamDatasets() {
    if (datasetCount == 0) {
        Serial.println("No datasets to stream!");
        return;
    }

    // Stream each dataset with clear separation
    for (int datasetIndex = 0; datasetIndex < datasetCount; datasetIndex++) {
        Dataset currentDataset;
        memcpy_P(&currentDataset, &storedDatasets[datasetIndex], sizeof(Dataset));
        
        // Start marker for each recording with its label and index
        Serial.print("START_RECORDING,");
        Serial.print(datasetIndex);
        Serial.print(",");
        Serial.println(currentDataset.label);
        
        // Send CSV header for this recording
        Serial.println("Sample,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ");
        
        // Send the 256 samples
        for (int i = 0; i < SAMPLES; i++) {
            Serial.print(i);
            for (int j = 0; j < FEATURES_PER_SAMPLE; j++) {
                Serial.print(",");
                Serial.print(currentDataset.data[i][j], 6);
            }
            Serial.println();
        }
        
        // End marker for this recording
        Serial.println("END_RECORDING");
        Serial.println(); // Empty line for better readability
    }
}

void viewStoredDatasets() {
    if (datasetCount == 0) {
        Serial.println("No datasets stored!");
        return;
    }

    Serial.println("\n=== Stored Datasets ===");
    for (int i = 0; i < datasetCount; i++) {
        Dataset currentDataset;
        memcpy_P(&currentDataset, &storedDatasets[i], sizeof(Dataset));
        
        Serial.print("Dataset ");
        Serial.print(i + 1);
        Serial.print(" - Label: ");
        Serial.print(currentDataset.label);
        Serial.print(" - First sample values: ");
        for (int j = 0; j < FEATURES_PER_SAMPLE; j++) {
            Serial.print(currentDataset.data[0][j], 4);
            Serial.print(" ");
        }
        Serial.println();
    }
    printMenu();
}