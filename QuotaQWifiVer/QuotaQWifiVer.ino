#include <Arduino.h>
#include <WiFi.h>

#define C1 A1
#define C2 A2
#define button D10
#define IA D9
#define IB D8

bool buttonstate = false;

// ===== WIFI INFO =====
const char* ssid = "Rev Member";
const char* password = "incubator";

// ===== WEB SERVER =====
WiFiServer server(80);

void setup() {
  delay(3000);
  pinMode(C1,INPUT);
  pinMode(C2,INPUT);
  pinMode(button,INPUT);
  pinMode(IA,OUTPUT);
  pinMode(IB,OUTPUT);
  Serial.begin(115200);

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
          if (request.indexOf("GET /H") >= 0&& !buttonstate) {
            buttonstate = true;
            analogWrite(IA,200);
            delay(3200);
            analogWrite(IA,0);
            analogWrite(IB,0);
          }

          // ===== TURN LED OFF =====
          if (request.indexOf("GET /L") >= 0 && buttonstate) {
            buttonstate = false;
            analogWrite(IB,200);
            delay(3200);
            analogWrite(IB,0);
            analogWrite(IA,0);
          }

          // ===== SEND WEBPAGE =====
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          client.println("<html>");
          client.println("<head><title>QuotaQuom</title></head>");
          client.println("<body>");

          client.println("<h1>QuotaQuom</h1>");

          client.println("<p><a href=\"/H\"><button>OUT</button></a></p>");
          client.println("<p><a href=\"/L\"><button>IN</button></a></p>");

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