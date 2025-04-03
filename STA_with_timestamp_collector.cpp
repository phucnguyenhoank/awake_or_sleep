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

// Wi-Fi credentials
const char* ssid = "tesla2";      // New SSID
const char* password = "teslatesla";

// Server addresses (ensure these are reachable in your new network)
const char *isCollectingDataUrl = "http://192.168.243.230:5000/is_collecting_data";
const char *dataUrl = "http://192.168.243.230:5000/data";

// Buffer
struct SensorData {
  unsigned long timestamp;  // Added timestamp field
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  long ir;
};

const int BUFFER_SIZE = 300;
SensorData buffer[BUFFER_SIZE];
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Status: ");
  Serial.println(WiFi.status());

  Wire.begin(SDA_PIN, SCL_PIN); // SDA, SCL pins for ESP32-C3

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

  // Check server command once at startup
  checkIsCollecting();
}

void loop() {
  unsigned long currentTime = millis();

  if (!collectingData) {
    checkIsCollecting();
    delay(500);
  } else if (currentTime - lastSampleTime >= 20) {

    lastSampleTime = currentTime;
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();

    // Include timestamp in the buffer
    buffer[bufferIndex] = {currentTime, a.acceleration.x, a.acceleration.y, a.acceleration.z,
                           g.gyro.x, g.gyro.y, g.gyro.z, irValue};
    bufferIndex++;
    if (bufferIndex == BUFFER_SIZE) {
      sendData();
      bufferIndex = 0;
      checkIsCollecting();
    }
  }
}


void checkIsCollecting() {
  // Serial.println(WiFi.ping("192.168.243.230"));
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

  // Allocate JSON document with updated capacity for 8 fields
  const size_t capacity = JSON_ARRAY_SIZE(bufferIndex) + bufferIndex * JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);

  // Create JSON array
  JsonArray dataArray = doc.to<JsonArray>();
  for (int i = 0; i < bufferIndex; i++) {
    JsonObject entry = dataArray.createNestedObject();
    entry["Timestamp"] = buffer[i].timestamp;  // Add timestamp to JSON
    entry["AccelX"] = buffer[i].accelX;
    entry["AccelY"] = buffer[i].accelY;
    entry["AccelZ"] = buffer[i].accelZ;
    entry["GyroX"] = buffer[i].gyroX;
    entry["GyroY"] = buffer[i].gyroY;
    entry["GyroZ"] = buffer[i].gyroZ;
    entry["IR"] = buffer[i].ir;
  }

  // Serialize to string
  String json;
  serializeJson(doc, json);

  // Send POST request
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
