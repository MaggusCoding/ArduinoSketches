// Define the pin to test
const int testPin = 13; // Change this if you suspect a different pin

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial); // Wait for serial port to connect

  // Set the test pin as an input with an internal pull-up resistor
  pinMode(testPin, INPUT_PULLUP);

  // Print initial message
  Serial.println("Testing pin 13. Press the button to see the output.");
}

void loop() {
  // Read the state of the pin
  int pinState = digitalRead(testPin);

  // Print the state of the pin
  Serial.print("Pin ");
  Serial.print(testPin);
  Serial.print(": ");
  Serial.println(pinState == LOW ? "LOW (Button Pressed)" : "HIGH (Button Not Pressed)");

  // Delay to prevent rapid serial output
  delay(500);
}
