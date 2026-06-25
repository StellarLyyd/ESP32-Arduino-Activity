#include <Arduino.h>
#include "BLEDevice.h"

static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9A");
static BLEUUID txCharUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9A");
static BLEUUID rxCharUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9A");

static bool doConnect = false;
static bool connected = false;

static BLEAdvertisedDevice *myDevice = nullptr;
static BLERemoteCharacteristic *pTxRemoteCharacteristic = nullptr;
static BLERemoteCharacteristic *pRxRemoteCharacteristic = nullptr;

static void notifyCallback(
  BLERemoteCharacteristic *pBLERemoteCharacteristic,
  uint8_t *pData,
  size_t length,
  bool isNotify
) {
  Serial.print("Received from server: ");
  Serial.write(pData, length);
  Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("Connected to server");
  }

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("Disconnected from server");
  }
};

bool connectToServer() {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    return false;
  }

  Serial.println("Connected");

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find UART service");
    pClient->disconnect();
    return false;
  }

  Serial.println("Found UART service");

  pTxRemoteCharacteristic = pRemoteService->getCharacteristic(txCharUUID);
  if (pTxRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find TX notify characteristic");
    pClient->disconnect();
    return false;
  }

  Serial.println("Found TX notify characteristic");

  if (pTxRemoteCharacteristic->canNotify()) {
    pTxRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Subscribed to notifications");
  } else {
    Serial.println("TX characteristic cannot notify");
  }

  pRxRemoteCharacteristic = pRemoteService->getCharacteristic(rxCharUUID);
  if (pRxRemoteCharacteristic != nullptr) {
    Serial.println("Found RX write characteristic");
  }

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveName() &&
        advertisedDevice.getName() == "PaShield") {

      Serial.println("Found target server by name!");

      BLEDevice::getScan()->stop();

      if (myDevice != nullptr) {
        delete myDevice;
      }

      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting BLE client...");

  BLEDevice::init("ESP32 BLE Client");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("Scanning...");
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    doConnect = false;

    if (connectToServer()) {
      Serial.println("Ready to receive data.");
    } else {
      Serial.println("Connection failed. Scanning again...");
      BLEDevice::getScan()->start(5, false);
    }
  }

  if (!connected && !doConnect) {
    BLEDevice::getScan()->start(5, false);
  }

  if (connected && pRxRemoteCharacteristic != nullptr) {
    String message = "Hello from client";
    pRxRemoteCharacteristic->writeValue(message.c_str(), message.length());
    delay(3000);
  }

  delay(100);
}