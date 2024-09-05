#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h> // Library for onboard IMU sensor
#include <vector> // Include vector for dynamic data storage

BLEService motionService("fbb9d115-a253-4636-9f9c-9be38930a78a"); // BLE service
BLECharacteristic accelDataChar("d1251add-8f4e-4ced-b172-04a4c8d66a1b", BLERead | BLENotify, 16); // Data characteristic
BLECharacteristic controlChar("19b10001-e8f2-537e-4f6c-d104768a1214", BLEWrite, 1); // Control characteristic for start

bool recording = false;
unsigned long recordingStartTime = 0;
const int recordingDuration = 10000; // 10 seconds

struct AccelData {
  float timestamp; // Timestamp in seconds
  float x, y, z;
};

std::vector<AccelData> dataCache; // Vector to dynamically store cached data

void setup() {
  Serial.begin(9600);  // Start serial communication at 9600 baud
  delay(1000);  // Give time for the serial connection to initialize (especially important when running on power only)

  // Only wait for the Serial Monitor if it's available
  if (Serial) {
    while (!Serial); // Wait for Serial Monitor to be opened if connected via USB
  }

  // Initialize IMU
  if (!IMU.begin()) {
    if (Serial) {
      Serial.println("Failed to initialize IMU!");
    }
    while (1); // Halt execution if IMU fails
  }

  // Initialize BLE
  if (!BLE.begin()) {
    if (Serial) {
      Serial.println("Starting BLE failed!");
    }
    while (1); // Halt execution if BLE fails
  }

  BLE.setLocalName("ArduinoBLE33Sense");
  BLE.setAdvertisedService(motionService);
  motionService.addCharacteristic(accelDataChar);
  motionService.addCharacteristic(controlChar);
  BLE.addService(motionService);
  BLE.advertise();

  if (Serial) {
    Serial.println("Bluetooth device active, waiting for connections...");
  }
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    if (Serial) {
      Serial.print("Connected to central: ");
      Serial.println(central.address());
    }

    while (central.connected()) {
      if (controlChar.written()) {
        byte controlValue = controlChar.value()[0]; // Start recording command received
        if (controlValue == 1) {
          startRecording();
        }
      }

      if (recording) {
        if (millis() - recordingStartTime < recordingDuration) {
          collectData();
        } else {
          recording = false;
          sendCachedData(); // Send data once recording duration is over
        }
      }
    }

    if (Serial) {
      Serial.print("Disconnected from central: ");
      Serial.println(central.address());
    }
  }
}

void startRecording() {
  recording = true;
  recordingStartTime = millis();
  dataCache.clear(); // Clear the vector for a new recording session
  if (Serial) {
    Serial.println("Recording started");
  }
}

void sendCachedData() {
  if (Serial) {
    Serial.println("Recording completed, sending cached data...");
  }

  // Send cached data over BLE
  for (const auto &data : dataCache) {
    byte buffer[16];
    memcpy(buffer, &data.timestamp, 4); // Copy timestamp
    memcpy(buffer + 4, &data.x, 4);     // Copy X
    memcpy(buffer + 8, &data.y, 4);     // Copy Y
    memcpy(buffer + 12, &data.z, 4);    // Copy Z

    accelDataChar.writeValue(buffer, sizeof(buffer));
    delay(50); // Small delay between sending data points
  }

  if (Serial) {
    Serial.println("All data sent");
  }
}

void collectData() {
  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    // Calculate the timestamp relative to the start of recording (in seconds)
    float currentTime = (millis() - recordingStartTime) / 1000.0; // Convert to seconds

    AccelData newData;
    newData.timestamp = currentTime; // Timestamp starts at 0 and progresses
    newData.x = x;
    newData.y = y;
    newData.z = z;

    // Add the data to the vector
    dataCache.push_back(newData);
  }
}
