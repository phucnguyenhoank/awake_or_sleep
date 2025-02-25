#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "random_forest_model.h"  // Your exported Random Forest
#include "scaler_values.h"        // Your scaler values

#define SDA_PIN 8  // ESP32-C3 SDA
#define SCL_PIN 9  // ESP32-C3 SCL

#define RED_PIN   2
#define GREEN_PIN 3
#define BLUE_PIN  4

Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

// Structure to hold a tree's data
struct Tree {
    int n_nodes;
    const int* children_left;
    const int* children_right;
    const int* feature;
    const float* threshold;
    const int* class_pred;
};

// Array of trees
Tree trees[] = {
    { tree_0_n_nodes, tree_0_children_left, tree_0_children_right, tree_0_feature, tree_0_threshold, tree_0_class },
    { tree_1_n_nodes, tree_1_children_left, tree_1_children_right, tree_1_feature, tree_1_threshold, tree_1_class },
    { tree_2_n_nodes, tree_2_children_left, tree_2_children_right, tree_2_feature, tree_2_threshold, tree_2_class },
    { tree_3_n_nodes, tree_3_children_left, tree_3_children_right, tree_3_feature, tree_3_threshold, tree_3_class },
    { tree_4_n_nodes, tree_4_children_left, tree_4_children_right, tree_4_feature, tree_4_threshold, tree_4_class }
};

// Predict with a single tree
int predict_tree(const Tree& tree, float* features) {
    int node = 0;
    while (tree.feature[node] != -2) {  // -2 indicates a leaf node
        int feature_idx = tree.feature[node];
        if (features[feature_idx] <= tree.threshold[node]) {
            node = tree.children_left[node];
        } else {
            node = tree.children_right[node];
        }
    }
    return tree.class_pred[node];
}

// Random Forest prediction (majority vote)
int predict(float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ, float ir) {
    // Normalize inputs
    float features[7];
    features[0] = (accelX - AccelX_min) / (AccelX_max - AccelX_min);
    features[1] = (accelY - AccelY_min) / (AccelY_max - AccelY_min);
    features[2] = (accelZ - AccelZ_min) / (AccelZ_max - AccelZ_min);
    features[3] = (gyroX - GyroX_min) / (GyroX_max - GyroX_min);
    features[4] = (gyroY - GyroY_min) / (GyroY_max - GyroY_min);
    features[5] = (gyroZ - GyroZ_min) / (GyroZ_max - GyroZ_min);
    features[6] = (ir - IR_min) / (IR_max - IR_min);

    // Vote across all trees
    int votes[2] = {0, 0};  // 0: Sleep, 1: Awake
    for (int i = 0; i < n_trees; i++) {
        int pred = predict_tree(trees[i], features);
        votes[pred]++;
    }
    return votes[1] > votes[0] ? 1 : 0;  // Majority vote
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize MPU6050
    if (!mpu.begin()) {
        Serial.println("MPU6050 not found!");
        while (1) { delay(10); }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 initialized");

    // Initialize MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30102 not found!");
        while (1) { delay(10); }
    }
    particleSensor.setup(0x1F, 1, 2, 400, 411, 4096);  // Mode 2: Red + IR
    particleSensor.clearFIFO();
    Serial.println("MAX30102 initialized");

    // RGB LED
    // Set RGB pins as output
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    
}

void loop() {
    
    // Read sensor data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();

    // Predict
    int result = predict(a.acceleration.x, a.acceleration.y, a.acceleration.z,
                        g.gyro.x, g.gyro.y, g.gyro.z, (float)irValue);

    // Output
    Serial.print("Prediction: ");
    Serial.println(result ? "Awake" : "Sleep");

    if (result) {
      setColor(0, 255, 0, 10);
    }
    else{
      setColor(0, 0, 255, 10);
    }

    delay(1000);
}

// Function to set color with brightness control
void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
  analogWrite(RED_PIN, (red * brightness) / 255);
  analogWrite(GREEN_PIN, (green * brightness) / 255);
  analogWrite(BLUE_PIN, (blue * brightness) / 255);
}