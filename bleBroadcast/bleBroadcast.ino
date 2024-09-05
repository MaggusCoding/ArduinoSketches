#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h> // Library for onboard IMU sensor

BLEService motionService("fbb9d115-a253-4636-9f9c-9be38930a78a"); // BLE service
BLECharacteristic accelDataChar("d1251add-8f4e-4ced-b172-04a4c8d66a1b", BLERead | BLENotify, 16); // 4 bytes for timestamp + 3 * 4 bytes for float

void setup() {
  Serial.begin(9600);  // Start serial communication at 9600 baud
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("ArduinoBLE33Sense");
  BLE.setAdvertisedService(motionService);
  motionService.addCharacteristic(accelDataChar);
  BLE.addService(motionService);
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");

  // Print the advertised service and characteristics UUIDs
  printAdvertisedUUIDs();
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      float x, y, z;
      if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(x, y, z);

        uint32_t timestamp = micros(); // Get timestamp in microseconds
        byte data[16];

        // Copy timestamp into the first 4 bytes of data array
        memcpy(data, &timestamp, 4);
        // Copy x, y, z into subsequent bytes of data array
        memcpy(data + 4, &x, 4);
        memcpy(data + 8, &y, 4);
        memcpy(data + 12, &z, 4);

        accelDataChar.writeValue(data, sizeof(data));

        delay(100); // Send data every 100ms
      }
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void printAdvertisedUUIDs() {
  // Print the service UUID
  Serial.print("Service UUID: ");
  Serial.println(motionService.uuid());

  // Print the characteristic UUIDs
  Serial.print("Characteristic Data UUID: ");
  Serial.println(accelDataChar.uuid());
}
