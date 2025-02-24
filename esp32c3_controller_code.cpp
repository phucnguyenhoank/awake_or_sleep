#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"

// Wi-Fi credentials
const char* ssid = "P912_2.4G";
const char* password = "12344321";

// Server endpoints
const char* commandUrl = "http://192.168.1.9:5000/command";
const char* dataUrl = "http://192.168.1.9:5000/data";

// Define I2C pins for ESP32-C3
#define SDA_PIN 8
#define SCL_PIN 9

// Buffer settings
#define SAMPLE_RATE_HZ 50  // 50 Hz sampling
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_RATE_HZ)  // 20 ms
#define BATCH_SIZE 50      // Send 10 samples per batch
#define BATCH_INTERVAL_MS (BATCH_SIZE * SAMPLE_INTERVAL_MS)  // 200 ms

// Data structure for a single sample
struct SensorData {
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  long ir, red;
};

// Buffer to store samples
SensorData buffer[BATCH_SIZE];
int bufferIndex = 0;

Adafruit_MPU6050 mpu;
MAX30105 particleSensor;
bool sendingData = false;
unsigned long lastBatchSent = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with specified pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println(WiFi.localIP());
  
  // Initialize MPU6050 sensor
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found! Check wiring and I2C address.");
    while (1) { delay(10); }
  }
  Serial.println("MAX30105 Found!");
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check server command periodically (every 1 second when not sending)
  if (!sendingData || (currentTime - lastBatchSent >= 1000)) {
    checkServerCommand();
  }
  
  if (sendingData) {
    // Collect data at 50 Hz
    collectSensorData();
    
    // Send batch every 200 ms
    if (bufferIndex >= BATCH_SIZE || (currentTime - lastBatchSent >= BATCH_INTERVAL_MS)) {
      sendSensorDataBatch();
      bufferIndex = 0;  // Reset buffer index
      lastBatchSent = currentTime;
    }
  } else {
    delay(1000);  // Idle delay
  }
}

void checkServerCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(commandUrl);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String response = http.getString();
      if (response == "start") {
        sendingData = true;
        bufferIndex = 0;  // Reset buffer when starting
        lastBatchSent = millis();
        //Serial.println("Start Command Received");
      } else if (response == "stop") {
        sendingData = false;
        Serial.println("Stop Command Received");
      }
    } else {
      Serial.print("Error checking command: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void collectSensorData() {
  if (bufferIndex < BATCH_SIZE) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();
    
    buffer[bufferIndex] = {a.acceleration.x, a.acceleration.y, a.acceleration.z,
                           g.gyro.x, g.gyro.y, g.gyro.z,
                           irValue, redValue};
    bufferIndex++;
  }
  delay(SAMPLE_INTERVAL_MS);  // Maintain 50 Hz
}

void sendSensorDataBatch() {
  if (WiFi.status() != WL_CONNECTED || bufferIndex == 0) return;
  
  HTTPClient http;
  http.begin(dataUrl);
  http.addHeader("Content-Type", "application/json");
  
  // Build JSON array
  String jsonData = "[";
  for (int i = 0; i < bufferIndex; i++) {
    if (i > 0) jsonData += ",";
    jsonData += "{";
    jsonData += "\"AccelX\":" + String(buffer[i].accelX, 2) + ",";
    jsonData += "\"AccelY\":" + String(buffer[i].accelY, 2) + ",";
    jsonData += "\"AccelZ\":" + String(buffer[i].accelZ, 2) + ",";
    jsonData += "\"GyroX\":" + String(buffer[i].gyroX, 2) + ",";
    jsonData += "\"GyroY\":" + String(buffer[i].gyroY, 2) + ",";
    jsonData += "\"GyroZ\":" + String(buffer[i].gyroZ, 2) + ",";
    jsonData += "\"IR\":" + String(buffer[i].ir) + ",";
    jsonData += "\"Red\":" + String(buffer[i].red);
    jsonData += "}";
  }
  jsonData += "]";
  
  int httpResponseCode = http.POST(jsonData);
  if (httpResponseCode > 0) {
    Serial.print("Batch sent (");
    Serial.print(bufferIndex);
    Serial.println(" samples)");
  } else {
    Serial.print("Error sending batch: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}