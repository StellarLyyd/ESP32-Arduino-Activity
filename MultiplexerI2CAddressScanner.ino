#include <Wire.h>

#define SDA_PIN 19
#define SCL_PIN 18

void scanBus() {
  int count = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      count++;
    }
  }

  if (count == 0) Serial.println("No devices found.");
}

void selectPCA9548A(uint8_t muxAddr, uint8_t channel) {
  Wire.beginTransmission(muxAddr);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Scanning main I2C bus...");
  scanBus();

  for (byte muxAddr = 0x70; muxAddr <= 0x77; muxAddr++) {
    Wire.beginTransmission(muxAddr);
    if (Wire.endTransmission() == 0) {
      Serial.print("\nPCA9548A found at 0x");
      Serial.println(muxAddr, HEX);

      for (uint8_t ch = 0; ch < 8; ch++) {
        Serial.print("Scanning channel ");
        Serial.println(ch);

        selectPCA9548A(muxAddr, ch);
        delay(20);
        scanBus();
        Serial.println();
      }
    }
  }
}

void loop() {
}