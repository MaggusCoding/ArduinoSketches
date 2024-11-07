#include <Arduino_LSM9DS1.h>

// Constants for data collection and FFT
const int SAMPLES = 256;              // Power of 2 for FFT
const int SAMPLING_FREQUENCY = 100;   // 100 Hz sampling rate
const int SAMPLING_PERIOD_US = 1000000 / SAMPLING_FREQUENCY;
const float COLLECTION_TIME_SEC = float(SAMPLES) / SAMPLING_FREQUENCY;

// Maximum number of datasets to store in PROGMEM
const int MAX_DATASETS = 10;
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
Dataset PROGMEM storedDatasets[MAX_DATASETS];

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  printMenu();
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    
    switch (command) {
      case 'c':
      case 'C':
        collectAndLabelData();
        break;
      case 'v':
      case 'V':
        viewStoredDatasets();
        break;
      case 's':
      case 'S':
        streamDatasets();
        break;
      case 'h':
      case 'H':
        printMenu();
        break;
      default:
        break;
    }
  }
}

void printMenu() {
  Serial.println("\n=== Motion Data Collection Menu ===");
  Serial.println("C: Collect new labeled dataset");
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

  // Send start marker and number of datasets
  Serial.println("START_STREAM");
  Serial.println(datasetCount);

  // Stream each dataset
  for (int datasetIndex = 0; datasetIndex < datasetCount; datasetIndex++) {
    Dataset currentDataset;
    memcpy_P(&currentDataset, &storedDatasets[datasetIndex], sizeof(Dataset));
    
    // Send dataset header
    Serial.print("DATASET_");
    Serial.print(datasetIndex);
    Serial.print("_LABEL_");
    Serial.println(currentDataset.label);

    // Send CSV header
    Serial.println("Sample,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ");
    
    // Send dataset values
    for (int i = 0; i < SAMPLES; i++) {
      Serial.print(i);
      for (int j = 0; j < FEATURES_PER_SAMPLE; j++) {
        Serial.print(",");
        Serial.print(currentDataset.data[i][j], 6);
      }
      Serial.println();
    }
    
    // Send dataset end marker
    Serial.println("END_DATASET");
  }

  // Send end stream marker
  Serial.println("END_STREAM");
}

void collectAndLabelData() {
  if (datasetCount >= MAX_DATASETS) {
    Serial.println("Error: Maximum number of datasets reached!");
    return;
  }

  // Collect the data
  Serial.println("\nStarting data collection...");
  collectSamples();
  
  // Get label from user
  Serial.println("\nEnter label (0: no theft, 1: theft attempt):");
  while (!Serial.available());
  
  int label = -1;
  while (label != 0 && label != 1) {
    if (Serial.available()) {
      char input = Serial.read();
      if (input == '0' || input == '1') {
        label = input - '0';
      }
    }
  }

  // Create new dataset
  Dataset newDataset;
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
  datasetCount++;

  Serial.print("Dataset stored with label: ");
  Serial.println(label);
  printMenu();
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
  
  unsigned long actualCollectionTime = millis() - startTime;
  Serial.print("Collection completed in ");
  Serial.print(actualCollectionTime);
  Serial.println(" ms");
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