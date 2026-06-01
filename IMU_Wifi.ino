#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>

Adafruit_MPU6050 mpu;

const char* ssid = "Rev Member";
const char* password = "incubator";

WiFiServer server(80);

bool motionDetected = false;

float ax = 0, ay = 0, az = 0;
float gx = 0, gy = 0, gz = 0;
float temperature = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(19, 18); // SDA, SCL

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }

  Serial.println("MPU6050 Found!");

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(false);
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  // Only update values when motion is detected
  if (mpu.getMotionInterruptStatus()) {
    motionDetected = true;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;

    gx = g.gyro.x;
    gy = g.gyro.y;
    gz = g.gyro.z;

    temperature = temp.temperature;
  }

  WiFiClient client = server.available();

  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>ESP32 Motion Detection</title>");
    client.println("<meta http-equiv='refresh' content='0.5'>");
    client.println("</head>");
    client.println("<body>");

    client.println("<h1>ESP32 MPU6050 Motion Detector</h1>");

    client.println("<h2>Last Motion Reading</h2>");
    client.println("<p>Acceleration X: " + String(ax) + " m/s^2</p>");
    client.println("<p>Acceleration Y: " + String(ay) + " m/s^2</p>");
    client.println("<p>Acceleration Z: " + String(az) + " m/s^2</p>");

    client.println("<p>Gyro X: " + String(gx) + " rad/s</p>");
    client.println("<p>Gyro Y: " + String(gy) + " rad/s</p>");
    client.println("<p>Gyro Z: " + String(gz) + " rad/s</p>");

    client.println("<p>Temperature: " + String(temperature) + " degC</p>");

    client.println("</body>");
    client.println("</html>");

    client.stop();
  }

  delay(10);
}