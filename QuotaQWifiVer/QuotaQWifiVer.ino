#include <Arduino.h>
#include <WiFi.h>
#include "webpage.h"

#define C1 D5
#define C2 D6
#define button D10
#define IA D9
#define IB D8

bool buttonstate = false;
volatile long Encodervalue=0;

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

  attachInterrupt(digitalPinToInterrupt(C1), updateEncoder, RISING);
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

void updateEncoder()
{
  if (digitalRead(C1)> digitalRead(C2)){
    Encodervalue++;
  }
  else{
    Encodervalue--;
  }
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

          if (request.indexOf("GET /H") >= 0&& !buttonstate) {
            buttonstate = true;
            analogWrite(IB,0);
            while(Encodervalue<5000){
              analogWrite(IA,200);
            }
            analogWrite(IA,0);
          }

          if (request.indexOf("GET /L") >= 0 && buttonstate) {
            buttonstate = false;
            analogWrite(IA,0);
            while(Encodervalue>200){
              analogWrite(IB,200);
            }
            analogWrite(IB,0);
          }

          // ===== SEND WEBPAGE =====
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          client.print(HTML_START);

          client.println(Encodervalue);

          client.print(HTML_END);

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
