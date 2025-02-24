#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Wi-Fi credentials
const char* ssid = "awake_or_sleep";
const char* password = "phucansangdi";

// Server endpoints
const char* commandUrl = "http://192.168.245.230:5000/command";  // For checking start/stop command
const char* dataUrl = "http://192.168.245.230:5000/data";          // For sending sensor data

// Define I2C pins for ESP32-C3
#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_MPU6050 mpu;
bool sendingData = false;

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
  
  // Initialize MPU6050 sensor
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  Serial.println("MPU6050 Found!");
  
  // Configure sensor settings
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  // Check the server for start/stop commands
  checkServerCommand();
  
  if (sendingData) {
    sendSensorData();
    delay(500);  // Adjust delay as needed for your sample rate
  } else {
    delay(1000);
  }
}

// Function to check for start/stop command from the server
void checkServerCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(commandUrl);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String response = http.getString();
      if (response == "start") {
        sendingData = true;
        Serial.println("Start Command Received");
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

// Function to send MPU6050 sensor data to the server
void sendSensorData() {
  if (WiFi.status() == WL_CONNECTED) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    HTTPClient http;
    http.begin(dataUrl);
    http.addHeader("Content-Type", "application/json");
    
    String jsonData = "{";
    jsonData += "\"accel_x\":" + String(a.acceleration.x, 2) + ",";
    jsonData += "\"accel_y\":" + String(a.acceleration.y, 2) + ",";
    jsonData += "\"accel_z\":" + String(a.acceleration.z, 2) + ",";
    jsonData += "\"gyro_x\":" + String(g.gyro.x, 2) + ",";
    jsonData += "\"gyro_y\":" + String(g.gyro.y, 2) + ",";
    jsonData += "\"gyro_z\":" + String(g.gyro.z, 2);
    jsonData += "}";
    
    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      Serial.print("Data sent: ");
      Serial.println(jsonData);
    } else {
      Serial.print("Error sending data: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
