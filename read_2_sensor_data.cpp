#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"

// Define I2C pins for ESP32-C3
static const uint8_t SDA_PIN = 8;
static const uint8_t SCL_PIN = 9;

// Create sensor objects
Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
  delay(1000);

  // Print header for Serial Monitor/Plotter
  Serial.println("AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,IR,Red");

  // Initialize I2C with specified pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found! Check wiring and I2C address.");
    while (1) { delay(10); }
  }

  // Initialize MAX30105
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found! Check wiring and I2C address.");
    while (1) { delay(10); }
  }

  // Configure MAX30105
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // Low red LED value for testing
  particleSensor.setPulseAmplitudeGreen(0);  // Disable green LED
}

void loop() {
  // Read MPU6050 data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Read MAX30105 data
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  // Print all data as comma-separated values
  Serial.print(a.acceleration.x); Serial.print(",");
  Serial.print(a.acceleration.y); Serial.print(",");
  Serial.print(a.acceleration.z); Serial.print(",");
  Serial.print(g.gyro.x);         Serial.print(",");
  Serial.print(g.gyro.y);         Serial.print(",");
  Serial.print(g.gyro.z);         Serial.print(",");
  Serial.print(irValue);          Serial.print(",");
  Serial.println(redValue);

  // Delay for consistent sampling (20ms matches MAX30105 rate)
  delay(20); // Adjust as needed for your application
}
