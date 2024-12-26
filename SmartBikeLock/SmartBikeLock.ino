// SmartBikeLock.ino
#include "Communication.h"
#include "NeuralNetworkBikeLock.h"

Communication bleComm;
NeuralNetworkBikeLock NN;

const unsigned int layers[] = {3, 9, 9, 1};

float weights[] = {
  -0.676266,  3.154561, -1.76689 ,
   1.589422, -2.340522,  1.447924,
   0.291685, -1.222407,  0.669717,
  -1.059862,  2.059782, -1.113708,
  -1.790229,  1.472432, -1.903783,
  -5.094713,  7.437615, -5.033135,
   2.341339,  3.370419,  2.185228,
  -3.887402,  1.453663, -3.861217,
  -1.555083,  2.943702, -0.472324,

  -1.171853, -0.45975 , -0.986132, -0.583541, -1.250889, -1.064349, -0.656225, -0.689616, -0.570443,
  -5.30186 ,  1.078257,  0.864669, -2.917707, -2.280059, -2.018297,  1.577451, -3.758011, -4.153339,
  -0.556209, -0.998336, -0.80149 , -0.232561, -1.087017, -1.286771, -1.034251, -0.05806 , -0.415967,
  -1.475901, -0.039556,  0.144446, -0.485774, -0.041879,  0.955343, -1.492304, -0.577319, -0.466558,
  -0.307791, -0.624868, -0.733248, -0.572921,  1.156592,  9.843138, -2.721857, -0.064086, -1.642469,
  -0.824234, -0.440457,  0.180901, -0.683897, -0.487519,  0.189743, -1.430297,  0.238511, -0.824287,
   0.251094, -3.009409, -1.58829 ,  0.590185,  0.597326, -5.243015,  2.710771,  2.596604,  0.969508,
  -1.344488,  2.618552,  0.642735, -0.947158, -0.286999,  3.797427, -2.443925, -0.833397, -1.654542,
  -0.138234, -0.931373, -0.183022, -0.493784, -0.784119, -0.275703, -2.113665,  0.761188, -0.810006,

  -0.049101, -6.781154,  0.14872 , -2.332737, -4.983434, -1.396086, 10.86302 , -5.551509, -1.648114
};

void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    Serial.println("Starting SmartBikeLock...");
    
    if (!bleComm.begin()) {
        Serial.println("Failed to initialize BLE communication!");
        while (1);
    }
    
    NN.init(layers, weights, 4);
}

void loop() {
    bleComm.update();
    
    if (bleComm.getCurrentCommand() == Command::GET_WEIGHTS) {
        Serial.println("Received GET_WEIGHTS command");
        
        size_t numWeights = NN.getTotalWeights();
        float* currentWeights = new float[numWeights];
        
        if (NN.getWeights(currentWeights, numWeights)) {
            // Send weights in chunks
            bleComm.sendWeights(currentWeights, numWeights);
            Serial.println("Weights sent successfully");
        } else {
            Serial.println("Failed to get weights");
        }
        
        delete[] currentWeights;
    }

    static bool receiving = false;
    static float tempWeights[117];
    static unsigned long lastReceiveTime = 0;
    
    if (bleComm.getCurrentCommand() == Command::SET_WEIGHTS) {
         // Only initialize receiving state when we first get the command
        if (!receiving) {
            Serial.println("Starting to receive new weights...");
            receiving = true;
            lastReceiveTime = millis();
            // Don't reset state here
        }
        
        // Check for timeout
        if (receiving && (millis() - lastReceiveTime > 5000)) {
            Serial.println("Timeout receiving weights. Resetting state.");
            receiving = false;
            bleComm.resetState();
            return;
        }
        
        // Try to receive weights
        if (bleComm.receiveWeights(tempWeights, 117)) {
            Serial.println("Successfully received all weights");
            if (NN.updateNetworkWeights(tempWeights, 117)) {
                Serial.println("Neural network weights updated");
            } else {
                Serial.println("Failed to update neural network weights");
            }
            receiving = false;
            // Only reset state after successful reception
            bleComm.resetState();
        }
        
        lastReceiveTime = millis();
    } else if (receiving) {
        // If we were receiving but command changed, clean up
        receiving = false;
        bleComm.resetState();
    }
    
    delay(50);  // Reduced delay for better responsiveness
}
