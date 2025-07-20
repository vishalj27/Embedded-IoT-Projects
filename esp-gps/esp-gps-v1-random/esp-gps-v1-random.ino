#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

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

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0);

#define GPS_BAUD 9600

const char* ssid = "NCAIR IOT";
const char* password = "Asim@123Tewari";
const char* server_name = "http://10.185.151.167:3300/api/location/create";  // FastAPI server

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

void setup() {
  SERIAL_BEGIN(115200);

  connectToWiFi();

  timeClient.begin();
  // timeClient.setTimeOffset(19800);

  mac_address = WiFi.macAddress();
  SERIAL_PRINT_LN(mac_address);

  Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  SERIAL_PRINT_LN("Serial 2 started at 9600 baud rate");
}

void loop() {

  connectToWiFi();

  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime(&epochTime);  // UTC

  sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
          ptm->tm_year + 1900,
          ptm->tm_mon + 1,
          ptm->tm_mday,
          ptm->tm_hour,
          ptm->tm_min,
          ptm->tm_sec);

  String json_data = "{";
  json_data += "\"name\": \"" + name + "\", ";
  json_data += "\"lat\": " + String(19.131648) + ", ";
  json_data += "\"long\": " + String(72.917619) + ", ";
  json_data += "\"timestamp\": \"" + String(timestamp) + "\", ";
  json_data += "\"mac_address\": \"" + mac_address + "\", ";
  json_data += "\"sats\": \"" + String(3) + "\", ";
  json_data += "\"alt\": " + String(1) + ", ";
  json_data += "\"speed\": " + String(1);
  json_data += "}";

  sendDataToServer(json_data);
  SERIAL_PRINT_LN(json_data);

  delay(1000);
}
