#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"  // MAX30105 library is compatible with MAX30102
#include <ArduinoJson.h>

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

// Global variables
bool collectingData = false;
unsigned long lastSampleTime = 0;

// Access Point (AP) Wi-Fi credentials
const char* ap_ssid = "ESP_AP";
const char* ap_password = "123456789";

// Server addresses (update these IPs based on the Flask server's IP on the AP network)
// For example, if your computer connected to the ESP32 AP gets IP 192.168.4.2:
const char *isCollectingDataUrl = "http://192.168.4.3:5000/is_collecting_data";
const char *dataUrl = "http://192.168.4.3:5000/data";

// Buffer structure for sensor data
struct SensorData {
  unsigned long timestamp;
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  long ir;
};

const int BUFFER_SIZE = 300;
SensorData buffer[BUFFER_SIZE];
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);

  // Set up ESP32 as an Access Point
  WiFi.mode(WIFI_AP);
  if (WiFi.softAP(ap_ssid, ap_password)) {
    Serial.println("AP Started successfully!");
    Serial.println("Access Point started");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP");
  }
  
  
  // Initialize I2C for sensors (ESP32-C3 uses designated SDA and SCL pins)
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found!");
    while (1) { delay(10); }
  }
  particleSensor.setup(0x1F, 1, 2, 400, 411, 4096);  // Mode 2: Red + IR
  particleSensor.clearFIFO();

  // Check the server command once at startup
  checkIsCollecting();
}

void loop() {
  unsigned long currentTime = millis();

  if (!collectingData) {
    // Continuously check the server command even in AP mode
    checkIsCollecting();
    delay(500);
  } else if (currentTime - lastSampleTime >= 20) {
    lastSampleTime = currentTime;
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();

    // Save sensor data along with timestamp in the buffer
    buffer[bufferIndex] = {currentTime, a.acceleration.x, a.acceleration.y, a.acceleration.z,
                           g.gyro.x, g.gyro.y, g.gyro.z, irValue};
    bufferIndex++;
    
    // When buffer is full, send data to the Flask server
    if (bufferIndex == BUFFER_SIZE) {
      sendData();
      bufferIndex = 0;
      checkIsCollecting();
    }
  }
}

void checkIsCollecting() {
  Serial.print("Number of HostConnections:");
  Serial.println(WiFi.softAPgetStationNum());
  HTTPClient http;
  http.begin(isCollectingDataUrl);
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String response = http.getString();
    if (response == "false") {
      collectingData = false;
      Serial.println("Finish collection.");
    } else {
      collectingData = true;
      Serial.println("Collecting...");
    }
  } else {
    Serial.print("Error checking command: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void sendData() {
  HTTPClient http;
  http.begin(dataUrl);
  http.addHeader("Content-Type", "application/json");

  // Create a JSON document that fits the data buffer
  const size_t capacity = JSON_ARRAY_SIZE(bufferIndex) + bufferIndex * JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);

  // Build the JSON array from the sensor data
  JsonArray dataArray = doc.to<JsonArray>();
  for (int i = 0; i < bufferIndex; i++) {
    JsonObject entry = dataArray.createNestedObject();
    entry["timestamp"] = buffer[i].timestamp;
    entry["AccelX"] = buffer[i].accelX;
    entry["AccelY"] = buffer[i].accelY;
    entry["AccelZ"] = buffer[i].accelZ;
    entry["GyroX"] = buffer[i].gyroX;
    entry["GyroY"] = buffer[i].gyroY;
    entry["GyroZ"] = buffer[i].gyroZ;
    entry["IR"] = buffer[i].ir;
  }

  // Serialize the JSON document to a string and send it via POST
  String json;
  serializeJson(doc, json);
  int code = http.POST(json);
  if (code > 0) {
    Serial.println("Sent " + String(bufferIndex) + " samples");
    Serial.print("Server response: ");
    Serial.println(http.getString());
  } else {
    Serial.print("Send error: ");
    Serial.println(code);
    Serial.println(http.errorToString(code));
  }
  http.end();
}
