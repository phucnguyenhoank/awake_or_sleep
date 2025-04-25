#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"  // MAX30105 library is compatible with MAX30102
#include <ArduinoJson.h>

#define SDA_PIN 8
#define SCL_PIN 9

#define RED_PIN   3
#define GREEN_PIN 4
#define BLUE_PIN  2

Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

// Global variables
bool collectingData = true;  // Set to true (or update logic as needed)
unsigned long lastSampleTime = 0;

// Wi-Fi credentials
const char* ssid = "newton";
const char* password = "newtonewton";

// UDP server parameters
const char* udpAddress = "192.168.65.230";  // Update to your server's IP
const int udpPort = 5001;
WiFiUDP udp;

// Buffer structure and global buffer
struct SensorData {
  unsigned long timestamp;
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  long ir;
};

const int BUFFER_SIZE = 400;
SensorData buffer[BUFFER_SIZE];
int bufferIndex = 0;

void setup() {
  // Serial.begin(115200);
  // delay(10);

  // Set RGB pins as output
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    setColor(0, 255, 0, 50);  // Green = trying
    delay(250);
    setColor(0, 0, 0, 50);  // Off
    delay(250);
  }
  delay(500);

  // Start UDP (optional – not strictly needed on the client side)
  udp.begin(WiFi.localIP(), udpPort);

  Wire.begin(SDA_PIN, SCL_PIN); // SDA, SCL pins for ESP32-C3

  // Initialize MPU6050
  if (!mpu.begin()) {
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    while (1) { delay(10); }
  }
  particleSensor.setup(0x1F, 1, 2, 400, 411, 4096);  // Mode 2: Red + IR
  particleSensor.clearFIFO();
}

void loop() {
  unsigned long currentTime = millis();

  if (collectingData && currentTime - lastSampleTime >= 20) {
    lastSampleTime = currentTime;
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();

    // Save sensor readings in buffer with a timestamp
    buffer[bufferIndex] = {currentTime, a.acceleration.x, a.acceleration.y, a.acceleration.z,
                           g.gyro.x, g.gyro.y, g.gyro.z, irValue};
    bufferIndex++;

    if (bufferIndex >= BUFFER_SIZE) {
      sendData();
      bufferIndex = 0;
    }
  }
}

void sendData() {
  // Allocate JSON document with an estimate for capacity
  const size_t capacity = JSON_ARRAY_SIZE(bufferIndex) + bufferIndex * JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);

  // Create a JSON array and add buffered sensor data
  JsonArray dataArray = doc.to<JsonArray>();
  for (int i = 0; i < bufferIndex; i++) {
    JsonObject entry = dataArray.createNestedObject();
    entry["Timestamp"] = buffer[i].timestamp;
    entry["AccelX"] = buffer[i].accelX;
    entry["AccelY"] = buffer[i].accelY;
    entry["AccelZ"] = buffer[i].accelZ;
    entry["GyroX"] = buffer[i].gyroX;
    entry["GyroY"] = buffer[i].gyroY;
    entry["GyroZ"] = buffer[i].gyroZ;
    entry["IR"] = buffer[i].ir;
  }

  // Serialize JSON document to string
  String json;
  serializeJson(doc, json);

  // Send JSON data via UDP
  udp.beginPacket(udpAddress, udpPort);
  udp.write(json.c_str());
  udp.endPacket();
}

void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
  analogWrite(RED_PIN, (red * brightness) / 255);
  analogWrite(GREEN_PIN, (green * brightness) / 255);
  analogWrite(BLUE_PIN, (blue * brightness) / 255);
}
