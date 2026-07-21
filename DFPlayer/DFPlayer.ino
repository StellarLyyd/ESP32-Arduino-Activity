#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

// ESP32 connections
constexpr uint8_t PIN_MP3_TX = 7;  // ESP32 TX -> DFPlayer RX
constexpr uint8_t PIN_MP3_RX = 6;  // ESP32 RX <- DFPlayer TX

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini player;

void setup() {
  Serial.begin(115200);

  dfSerial.begin(
    9600,
    SERIAL_8N1,
    PIN_MP3_RX,
    PIN_MP3_TX
  );

  Serial.println("Starting DFPlayer...");

  if (!player.begin(dfSerial)) {
    Serial.println("DFPlayer initialization failed.");
    Serial.println("Check wiring and microSD card.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("DFPlayer ready.");

  player.volume(30);  // Range: 0–30
  player.play(1);     // Play track 0001.mp3
}

void loop() {
}