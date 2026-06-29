#include <Arduino.h>
#include <Wire.h>
#include "MS5837.h"

#define PCA9548A_ADDR 0x77   // your mux address

double diffq1 = 0;
double diffq2 = 0;
double baseline1 = 0;
double baseline2 = 0;
double depth = 0;

MS5803 MS(0x76);             // MS5837 sensor address

void selectMuxChannel(uint8_t channel) {
  if (channel > 7) return;

  Wire.beginTransmission(PCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(10);
}

void readSensor1(uint8_t channel) {
  selectMuxChannel(channel);

  int err = MS.read();

  Serial.print("Channel ");
  Serial.print(channel);
  Serial.print("  ");

  if (err != 0) {
    Serial.print("Read error: ");
    Serial.print(err);
    Serial.print("  Last error: ");
    Serial.println(MS.getLastError());
    return;
  }

  if(channel ==1){
    baseline1 = MS.getPressure();
  }else if(channel ==2){
    baseline2 = MS.getPressure();
  }
}

void readSensor(uint8_t channel) {
  selectMuxChannel(channel);

  int err = MS.read();

  Serial.print("Channel ");
  Serial.print(channel);
  Serial.print("  ");

  if (err != 0) {
    Serial.print("Read error: ");
    Serial.print(err);
    Serial.print("  Last error: ");
    Serial.println(MS.getLastError());
    return;
  }

  //Serial.print("Temp: ");
  //Serial.print(MS.getTemperature(), 2);
  Serial.print("Pressure: ");
  Serial.println(MS.getPressure(), 2);

  if(channel ==1){
    diffq1 = MS.getPressure() - baseline1 + 6.2;
    Serial.print("Difference: ");
    Serial.println(diffq1);
    depth = diffq1*38.8-1.63;
    Serial.print("Depth");
    Serial.println(depth);
  }else if(channel == 2){
    diffq2 = MS.getPressure() - baseline2;
    Serial.print("Difference: ");
    Serial.println(diffq2);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();

  Serial.println("Starting two MS5837 sensors through PCA9548A...");

  selectMuxChannel(1);
  if (MS.begin(2)) {
    Serial.println("Sensor on channel 1 found.");
    readSensor1(1);
  } else {
    Serial.println("Sensor on channel 1 not found.");
  }

  selectMuxChannel(2);
  if (MS.begin(2)) {
    Serial.println("Sensor on channel 2 found.");
    readSensor1(2);
    } else {
    Serial.println("Sensor on channel 2 not found.");
  }

  
}

void loop() {
  readSensor(1);
  delay(500);

  readSensor(2);
  delay(1000);

  Serial.println();
}