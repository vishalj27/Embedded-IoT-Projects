#include <WiFi.h>
#include <HTTPClient.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define DEBUG 1

#if DEBUG == 1
  #define SERIAL_BEGIN(X) Serial.begin(X)
  #define SERIAL_PRINT(X) Serial.print(X)
  #define SERIAL_PRINT_LN(X) Serial.println(X)
  #define SERIAL_PRINT_F(X) Serial.printf(X)
#else
  #define SERIAL_BEGIN(X)
  #define SERIAL_PRINT(X)
  #define SERIAL_PRINT_LN(X)
#endif

const String targetDevices[] = {"E001"};
const int numDevices = sizeof(targetDevices) / sizeof(targetDevices[0]);

const char* ssid = "";
const char* password = "";
String url = "";

static unsigned long lastScanAttempt = 0;
const unsigned long scanInterval = 1000;

HTTPClient http;

void connectToWifi() {

  if (WiFi.status() != WL_CONNECTED) {
    SERIAL_PRINT_LN("Attempting to connect Wi-Fi ...");
    // WiFi.disconnect();
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) { // Timeout after 10 seconds
      delay(500);
      SERIAL_PRINT(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      SERIAL_PRINT_LN("\nconnected to Wi-Fi");
      SERIAL_PRINT("IP Address: ");
      SERIAL_PRINT_LN(WiFi.localIP());
    } else {
      SERIAL_PRINT_LN("\nFailed to connect to Wi-Fi");
    }
  }
}

void sendJsonData(String deviceName, float temp, float hum) {

  // Create JSON payload
  char jsonPayload[200];
  int deviceId = 1;
  sprintf(jsonPayload,
    "{\"deviceid\": %d, \"data\": {\"temperature\": [{\"temp\": %.2f}], \"humidity\": [{\"percentage\": %.2f}]}}",
    deviceId, temp, hum);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    SERIAL_PRINT_LN(httpResponseCode);
    SERIAL_PRINT_LN(response);
  } else {
    SERIAL_PRINT("Error sending POST: ");
    SERIAL_PRINT_LN(httpResponseCode);
  }

  http.end();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {


    for (int i = 0; i < numDevices; i++) {

      if (advertisedDevice.getName() == targetDevices[i]) {

        SERIAL_PRINT("Found device: ");
        SERIAL_PRINT_LN(advertisedDevice.getName().c_str());


        if (advertisedDevice.haveManufacturerData()) {
          String manufacturerData = advertisedDevice.getManufacturerData();

          // Store MAC address
          char macAddress[18];
          strncpy(macAddress, advertisedDevice.getAddress().toString().c_str(), sizeof(macAddress) - 1);
          macAddress[sizeof(macAddress) - 1] = '\0';

          float temp = ((int16_t)((uint8_t)manufacturerData[3] << 8 | (uint8_t)manufacturerData[2])) / 100;
          float hum = ((int16_t)((uint8_t)manufacturerData[5] << 8 | (uint8_t)manufacturerData[4])) / 100;
          
          #if DEBUG
            char buffer[50];
            sprintf(buffer, "temp: %.2f, hum: %.2f", temp, hum);
            Serial.println(buffer);
          #endif

          sendJsonData(targetDevices[i], temp, hum);

        }

      }
    }
  }
};

void startBleInit() {
  BLEDevice::getScan()->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  BLEDevice::getScan()->setInterval(1349);
  BLEDevice::getScan()->setWindow(449);
  BLEDevice::getScan()->setActiveScan(true);
  BLEDevice::getScan()->start(5, false);
}

void startBLEScan() {
  BLEDevice::getScan()->start(5, false);
}

void setup() {
  SERIAL_BEGIN(115200);

  connectToWifi();
  SERIAL_PRINT_LN();
  SERIAL_PRINT_LN("Connected to the WiFi network");
  SERIAL_PRINT("My IP address: ");
  SERIAL_PRINT_LN(WiFi.localIP());

  BLEDevice::init("");
  startBleInit();
  startBLEScan();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED){
        connectToWifi();
      }

      unsigned long now = millis();
      if (now - lastScanAttempt > scanInterval) {

        lastScanAttempt = now;

        SERIAL_PRINT_LN("Scanning for advertisment packet !!");

        startBLEScan();

      }
}
