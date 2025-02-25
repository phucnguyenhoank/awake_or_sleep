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

// Global variables (declare these at the top of your sketch)
bool collectingData = false;
unsigned long lastSampleTime = 0;

// Wifi
const char* ssid = "P912_2.4G";
const char* password = "12344321";

// Server addresses
const char *isCollectingDataUrl = "http://192.168.1.12:5000/is_collecting_data"; // Replace with your URL
const char *dataUrl = "http://192.168.1.12:5000/data";              // URL for sending data

// Buffer
struct SensorData {
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  long ir;
};

const int BUFFER_SIZE = 300;   // Adjust based on your needs
SensorData buffer[BUFFER_SIZE]; // Buffer for 50 Hz data (assuming float data)
int bufferIndex = 0;

void setup()
{
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("\nWi-Fi connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");
  Serial.println(WiFi.localIP());

  Wire.begin(SDA_PIN, SCL_PIN); // SDA, SCL pins for ESP32-C3

  // Initialize MPU6050
  if (!mpu.begin())
  {
    Serial.println("MPU6050 not found!");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30102 not found!");
    while (1) { delay(10); }
  }
  particleSensor.setup(0x1F, 1, 2, 400, 411, 4096);  // Mode 2: Red + IR
  particleSensor.clearFIFO();

  // Check server command once at startup
  checkIsCollecting(); 
}

void loop()
{
  unsigned long currentTime = millis();
  
  // Serial.print("currentTime:");
  // Serial.println(currentTime);
  // Serial.print("lastSampleTime:");
  // Serial.println(lastSampleTime);
  // Serial.print("lastSampleTime:");
  // Serial.println(collectingData);
  // Serial.println("---------");

  if (!collectingData) // sampling with 50Hz
  {
    // Check if buffer is full
    // Serial.print("NO SENTTTTTT\n");
    checkIsCollecting(); // check if server want to collect data, change the collectingData to true to start collecting
    delay(500);
    
  }
  else if (currentTime - lastSampleTime >= 20)
  {
    lastSampleTime = currentTime;
    //Serial.print("SENTTTTTT\n");
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long irValue = particleSensor.getIR();
    buffer[bufferIndex] = {a.acceleration.x, a.acceleration.y, a.acceleration.z,
                            g.gyro.x, g.gyro.y, g.gyro.z,
                            irValue};
    bufferIndex++;
    // Check if buffer is full
    // Serial.print("bufferIndex:");
    // Serial.println(bufferIndex);
    if (bufferIndex == BUFFER_SIZE)
    {
      // Check if buffer is full
      //Serial.print("SENTTTTTT\n\n\n\n");
      sendData(); // server make sure this always available, even times up
      bufferIndex = 0; // Reset buffer after sending
      checkIsCollecting(); // check if times up, change the collectingData to false 
    }
    
  }
}

void checkIsCollecting()
{
  HTTPClient http;
  http.begin(isCollectingDataUrl);
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200)
  {
    String response = http.getString();
    if (response == "false")
    {
      collectingData = false;
      Serial.println("Finish collection.");
    }
    else
    {
      collectingData = true;
      Serial.println("Collecting...");
    }
  }
  else
  {
    Serial.print("Error checking command: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void sendData() {
  HTTPClient http;
  http.begin(dataUrl);
  http.addHeader("Content-Type", "application/json");

  // Debug: Check buffer size
  Serial.print("Sending buffer with ");
  Serial.print(bufferIndex);
  Serial.println(" samples");

  // Allocate JSON document
  const size_t capacity = JSON_ARRAY_SIZE(bufferIndex) + bufferIndex * JSON_OBJECT_SIZE(7);
  DynamicJsonDocument doc(capacity);

  // Create JSON array
  JsonArray dataArray = doc.to<JsonArray>();
  for (int i = 0; i < bufferIndex; i++) {
    JsonObject entry = dataArray.createNestedObject();
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

  // Debug: Print JSON payload
  // Serial.print("JSON payload: ");
  // Serial.println(json);
  // Serial.print("JSON length: ");
  // Serial.println(json.length());

  // Send POST request
  int code = http.POST(json);
  if (code > 0) {
    Serial.println("Sent " + String(bufferIndex) + " samples");
    Serial.print("Server response: ");
    Serial.println(http.getString());  // Print server response
  } else {
    Serial.print("Send error: ");
    Serial.println(code);
    Serial.println(http.errorToString(code));  // Detailed error
  }

  http.end();
}
