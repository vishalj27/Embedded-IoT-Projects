#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define DEBUG 1

#if DEBUG == 1
  #define SERIAL_BEGIN(x) Serial.begin(x)
  #define SERIAL_PRINT(x) Serial.print(x)
  #define SERIAL_PRINT_LN(x) Serial.println(x)
#else
  #define SERIAL_BEGIN(x)
  #define SERIAL_PRINT(x)
  #define SERIAL_PRINT_LN(x)
#endif

#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

const char* ssid = "Tarun";
const char* password = "ncair123";
const char* server_url = "https://10.185.151.167:3300/api/location/create";

TinyGPSPlus gps;
HTTPClient http;

String mac_address;
String device_name = "GPS_Device_2";

float prev_lat = 0.0;
float prev_lng = 0.0;
float prev_speed = 0.0;


struct GPSData {
  float lat;
  float lng;
  float alt;
  float speed;
  int sats;
  char timestamp[30];
};

// Function to connect to WiFi
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  SERIAL_PRINT_LN("Connecting to WiFi...");
  WiFi.disconnect();  // reset state
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    SERIAL_PRINT(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    SERIAL_PRINT_LN("\nWiFi Connected!");
  } else {
    SERIAL_PRINT_LN("\nWiFi Connection Failed.");
  }
}

// Function to read and parse GPS data
bool read_gps_data(GPSData &data) {
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  if (gps.location.isUpdated()) {
    data.lat = gps.location.lat();
    data.lng = gps.location.lng();
    data.alt = gps.altitude.meters();
    data.speed = gps.speed.kmph();
    data.sats = gps.satellites.value();

    sprintf(data.timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            gps.date.year(), gps.date.month(), gps.date.day(),
            gps.time.hour(), gps.time.minute(), gps.time.second());

    return true;
  }
  return false;
}

// Function to format JSON from GPS data
String create_json(const GPSData &data) {
  String json = "{";
  json += "\"name\": \"" + device_name + "\", ";
  json += "\"lat\": " + String(data.lat, 6) + ", ";
  json += "\"long\": " + String(data.lng, 6) + ", ";
  json += "\"timestamp\": \"" + String(data.timestamp) + "\", ";
  json += "\"mac_address\": \"" + mac_address + "\", ";
  json += "\"sats\": \"" + String(data.sats) + "\", ";
  json += "\"alt\": " + String(data.alt, 2) + ", ";
  json += "\"speed\": " + String(data.speed, 2);
  json += "}";
  return json;
}

// Function to send JSON data to server
void send_to_server(const String &json) {
  http.begin(server_url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(json);
  if (httpCode > 0) {
    String response = http.getString();
    SERIAL_PRINT_LN("HTTP Code: " + String(httpCode));
    SERIAL_PRINT_LN("Server: " + response);
  } else {
    SERIAL_PRINT_LN("POST Failed: " + String(httpCode));
  }

  http.end();
}

// Setup function
void setup() {
  SERIAL_BEGIN(115200);
  connectToWiFi();

  mac_address = WiFi.macAddress();
  SERIAL_PRINT_LN("MAC: " + mac_address);

  Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  SERIAL_PRINT_LN("GPS Serial started");
}

void loop() {
  connectToWiFi();

  GPSData gps_data;
  if (read_gps_data(gps_data)) {

    // if (gps_data.lat != prev_lat || gps_data.lng != prev_lng || gps_data.speed != prev_speed) {
      String json = create_json(gps_data);
      SERIAL_PRINT_LN("Sending JSON: " + json);
      send_to_server(json);

      prev_lat = gps_data.lat;
      prev_lng = gps_data.lng;
      prev_speed = gps_data.speed;
    // } else {
      // SERIAL_PRINT_LN("No change in GPS data. Skipping send.");
    // }
  }

  delay(1000);
}
