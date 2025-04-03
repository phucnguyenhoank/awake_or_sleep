#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"  // Use MAX30105 library for MAX30102 compatibility

// Pin definitions for ESP32-C3 (adjust as needed)
#define SDA_PIN 8  // Default SDA for ESP32-C3
#define SCL_PIN 9  // Default SCL for ESP32-C3

#define RED_PIN   3
#define GREEN_PIN 4
#define BLUE_PIN  2

// Sensor objects
Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

// Prediction function (your Decision Tree)
int predict(float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ, float ir) {
    if () {
        
    } else {
        
    }
}

void setup() {
    // Initialize serial communication
    // Serial.begin(115200);
    // while (!Serial) {
    //     delay(10);  // Wait for serial to connect
    // }

    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize MPU6050
    if (!mpu.begin()) {
        // Serial.println("MPU6050 not found!");
        while (1) { delay(10); }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    // Serial.println("MPU6050 initialized");

    // Initialize MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        // Serial.println("MAX30102 not found!");
        while (1) { delay(10); }
    }
    particleSensor.setup(0x1F, 1, 2, 400, 411, 4096);  // Mode 2: Red + IR
    particleSensor.clearFIFO();
    // Serial.println("MAX30102 initialized");

    // RGB LED
    // Set RGB pins as output
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
}

void loop() {
    // Get sensor data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();

    // Extract sensor readings
    float accelX = a.acceleration.x;
    float accelY = a.acceleration.y;
    float accelZ = a.acceleration.z;
    float gyroX = g.gyro.x;
    float gyroY = g.gyro.y;
    float gyroZ = g.gyro.z;
    float ir = (float)irValue;

    // Make prediction
    int result = predict(accelX, accelY, accelZ, gyroX, gyroY, gyroZ, ir);

    if (result) {
      setColor(0, 255, 0, 10);
    }
    else{
      setColor(0, 0, 255, 10);
    }

    delay(1000);  // Delay 1 second between predictions
}

// Function to set color with brightness control
void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
  analogWrite(RED_PIN, (red * brightness) / 255);
  analogWrite(GREEN_PIN, (green * brightness) / 255);
  analogWrite(BLUE_PIN, (blue * brightness) / 255);
}