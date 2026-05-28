#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h> // include the ESP32 servo library. Input Servo.h only for an Arduino
Servo myservo; // create a new Servo object
#define input1 A1
#define motor 5 
#define LED 9

int pos = 0;

// ===== WIFI INFO =====
const char* ssid = "Rev Member";
const char* password = "incubator";

// ===== WEB SERVER =====
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  delay(3000);

  pinMode(motor, OUTPUT); // declare servo signal port as the output
  pinMode(LED, OUTPUT); // declare LED as the output
  pinMode(input1, INPUT); // declare analog input port as the input
  
  ESP32PWM::allocateTimer(0); // set up a pwm timer; not always necessary
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(motor, 1000, 2000); // attach the servo object to the signal port

  Serial.println();
  Serial.println("Starting ESP32-C6...");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to WiFi
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Start server
  server.begin();

  Serial.println("Web server started!");
}

void loop() {

  // Check for incoming clients
  WiFiClient client = server.available();

  if (client) {

    Serial.println("New Client Connected");

    String request = "";

    while (client.connected()) {

      if (client.available()) {

        char c = client.read();

        Serial.write(c);

        request += c;

        // End of HTTP request
        if (c == '\n') {

          // ===== TURN LED ON =====
          if (request.indexOf("GET /H") >= 0) {
            digitalWrite(LED, LOW); // turn off the LED
            for (pos = 0; pos <= 180; pos += 1) {
		        myservo.write(pos);    // tell servo to go to position in variable 'pos'
		        delay(15);             // waits 15ms for the servo to reach the position
	          }
            for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
              myservo.write(pos);              // tell servo to go to position in variable 'pos'
              delay(15);                       // waits 15 ms for the servo to reach the position
            }
          }

          // ===== TURN LED OFF =====
          if (request.indexOf("GET /L") >= 0) {
              digitalWrite(LED, HIGH);
          }

          // ===== SEND WEBPAGE =====
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          client.println("<html>");
          client.println("<head><title>ESP32 LED Control</title></head>");
          client.println("<body>");

          client.println("<h1>ESP32-C6 Web Server</h1>");

          client.println("<p><a href=\"/H\"><button>Servo ON LED OFF</button></a></p>");
          client.println("<p><a href=\"/L\"><button>Servo OFF LED ON</button></a></p>");

          client.println("</body>");
          client.println("</html>");

          client.println();

          break;
        }
      }
    }

    // Close connection
    client.stop();

    Serial.println("Client Disconnected");
  }
}