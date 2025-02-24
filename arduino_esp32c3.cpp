#include <ArduinoJson.h>

// Global variables (declare these at the top of your sketch)
bool collectingData = false;
unsigned long lastSampleTime = 0;

const int BUFFER_SIZE = 100;   // Adjust based on your needs
float dataBuffer[BUFFER_SIZE]; // Buffer for 50 Hz data (assuming float data)
int bufferIndex = 0;

const char *finishTimeUrl = "http://your-server/finish-time"; // Replace with your URL
const char *dataUrl = "http://your-server/data";              // URL for sending data

void setup()
{
  Serial.begin(115200);
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");

  // Initialize MPU6050
  Wire.begin(8, 9); // SDA, SCL pins for ESP32-C3
  if (!mpu.begin())
  {
    Serial.println("MPU6050 not found!");
    while (1)
      delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30102 not found!");
    while (1)
      delay(10);
  }
  particleSensor.setup();                    // Default settings for MAX30102
  particleSensor.setPulseAmplitudeRed(0x0A); // Adjust Red LED amplitude
  particleSensor.setPulseAmplitudeGreen(0);  // Disable Green LED (not used in MAX30102)
  checkServerCommand();                      // Check server command once at startup
}

void loop()
{
  if (collectingData)
  {
    unsigned long currentTime = millis();

    // Collect data at 50 Hz (every 20 ms)
    if (currentTime - lastSampleTime >= 20)
    {
      lastSampleTime = currentTime;
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      long irValue = particleSensor.getIR();
      long redValue = particleSensor.getRed();
      buffer[bufferIndex] = {a.acceleration.x, a.acceleration.y, a.acceleration.z,
                             g.gyro.x, g.gyro.y, g.gyro.z,
                             irValue, redValue};
      bufferIndex++;

      // Check if buffer is full
      if (bufferIndex >= BUFFER_SIZE)
      {
        sendData();
        bufferIndex = 0; // Reset buffer after sending
      }
    }
    else 
    {
      checkIsCollecting();
      delay(1000);
    }
  }
}

void checkIsCollecting()
{
  HTTPClient http;
  http.begin(finishTimeUrl);
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200)
  {
    String response = http.getString();
    if (response == 'true')
    {
      collectingData = false;
      Serial.println("Finish collection.");
    }
  }
  else
  {
    Serial.print("Error checking command: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void sendData()
{
  HTTPClient http;
  http.begin(dataUrl);
  http.addHeader("Content-Type", "application/json");

  String json = "[";
  for (int i = 0; i < bufferIndex; i++)
  {
    if (i > 0)
      json += ",";
    json += "{\"AccelX\":" + String(buffer[i].accelX, 2) +
            ",\"AccelY\":" + String(buffer[i].accelY, 2) +
            ",\"AccelZ\":" + String(buffer[i].accelZ, 2) +
            ",\"GyroX\":" + String(buffer[i].gyroX, 2) +
            ",\"GyroY\":" + String(buffer[i].gyroY, 2) +
            ",\"GyroZ\":" + String(buffer[i].gyroZ, 2) +
            ",\"IR\":" + String(buffer[i].ir) +
            ",\"Red\":" + String(buffer[i].red) + "}";
  }
  json += "]";

  int code = http.POST(json);
  if (code > 0)
  {
    Serial.println("Sent " + String(bufferIndex) + " samples");
  }
  else
  {
    Serial.println("Send error: " + String(code));
  }
  http.end();
}