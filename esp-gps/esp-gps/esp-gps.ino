#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define DEBUG 1

#if DEBUG == 1
#define SERIAL_BEGIN(X) Serial.begin(X)
#define SERIAL_PRINT(X) Serial.print(X)
#define SERIAL_PRINT_LN(X) Serial.println(X)
#else
#define SERIAL_BEGIN(X)
#define SERIAL_PRINT(X)
#define SERIAL_PRINT_LN(X)
#endif

#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;


#define GPS_BAUD 9600

const char* ssid = "";
const char* password = "";
const char* server_name = "";  // FastAPI server

String mac_address;

HTTPClient http;

String name = "GPS_Device_1";

char timestamp[30];

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    SERIAL_PRINT_LN("WiFi not connected. Attempting to connect...");
    WiFi.disconnect();  // Ensure a clean state
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      SERIAL_PRINT(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      SERIAL_PRINT_LN("\nConnected to WiFi");
    } else {
      SERIAL_PRINT_LN("\nFailed to connect to WiFi");
    }
  }
}

void sendDataToServer(String json_string) {

  
    http.begin(server_name);
    http.addHeader("Content-Type", "application/json");
    // http.addHeader("User-Agent", "semIOE/2024.7");

    int httpResponseCode = http.POST(json_string);

    if (httpResponseCode > 0) {
      String response = http.getString();
      SERIAL_PRINT_LN("HTTP Response Code: " + String(httpResponseCode));
      SERIAL_PRINT_LN("Server Response: " + response);
    } else {
      SERIAL_PRINT_LN("Error sending POST: " + String(httpResponseCode));
    }

    http.end();
}

void setup(){
  SERIAL_BEGIN(115200);
  
  connectToWiFi();

  mac_address = WiFi.macAddress();
  SERIAL_PRINT_LN(mac_address);

  Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  SERIAL_PRINT_LN("Serial 2 started at 9600 baud rate");
}

void loop() {

    connectToWiFi();



    while (Serial2.available() > 0) {
      gps.encode(Serial2.read());
    }

    if (gps.location.isUpdated()) {
      String name = "GPS_Device_1";
      float lat = gps.location.lat();
      float lng = gps.location.lng();
      float alt = gps.altitude.meters();
      float speed = gps.speed.kmph();
      int sats = gps.satellites.value();

      char timestamp[30];
      sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
              gps.date.year(), gps.date.month(), gps.date.day(),
              gps.time.hour(), gps.time.minute(), gps.time.second());

      String json_data = "{";
      json_data += "\"name\": \"" + name + "\", ";
      json_data += "\"lat\": " + String(lat, 6) + ", ";
      json_data += "\"long\": " + String(lng, 6) + ", ";
      json_data += "\"timestamp\": \"" + String(timestamp) + "\", ";
      json_data += "\"mac_address\": \"" + mac_address + "\", ";
      json_data += "\"sats\": \"" + String(sats) + "\", ";
      json_data += "\"alt\": " + String(alt, 2) + ", ";
      json_data += "\"speed\": " + String(speed, 2);
      json_data += "}";

      sendDataToServer(json_data);
      SERIAL_PRINT_LN(json_data);
    }

    sendDataToServer(json_data);
    SERIAL_PRINT_LN(json_data);

    delay(1000);
}
