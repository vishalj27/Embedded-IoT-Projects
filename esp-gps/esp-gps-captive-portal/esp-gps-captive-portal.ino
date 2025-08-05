#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>

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

String ssid = "";
String password = "";
String end_point = "";

struct GPSData {
  float lat;
  float lng;
  float alt;
  float speed;
  int sats;
  char timestamp[30];
};

Preferences preferences;
TinyGPSPlus gps;
HTTPClient http;
AsyncWebServer server(80);

bool captive = false;
volatile bool cap = false;

TaskHandle_t TaskHandle_1;

void IRAM_ATTR handleInterrupt() {
  cap = true;

#if DEBUG
  Serial.println(captive);
  Serial.println("Interrupt activatead!!!!");
#endif
}


String mac_address;
String device_name = "GPS_Device_2";

float prev_lat = 0.0;
float prev_lng = 0.0;
float prev_speed = 0.0;


// Function to connect to WiFi
void wifi_check() {
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


void connectToWiFi(const String &ssid, const String &password) {
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.println("Connecting to WiFi...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Starting captive portal...");
    startCaptivePortal();
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

  wifi_check();

  http.begin(end_point);
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

  Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  SERIAL_PRINT_LN("GPS Serial started");

  preferences.begin("wifi", false);  // Initialize Preferences
  // Uncomment if you want to set default values initially (one-time setup)
  // preferences.putString("ssid", "");
  // preferences.putString("password", "");
  // preferences.putBool("captive", false);

  pinMode(4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(4), handleInterrupt, FALLING);

  // true when ISR interuptes
  captive = preferences.getBool("captive", false);
  Serial.print(captive);
  if (captive == true) {
    preferences.putString("ssid", "");
    preferences.putString("password", "");
    preferences.putString("endpoint", "");

    cap = false;
    captive = preferences.putBool("captive", false);
  }

  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  end_point = preferences.getString("endpoint", "");
  
  Serial.println("Stored SSID: " + ssid);
  Serial.println("Stored Password: " + password);
  Serial.println("Stored End point: " + end_point);


  if (ssid.isEmpty() || password.isEmpty() || end_point.isEmpty()) {
    startCaptivePortal();
  } else {
    connectToWiFi(ssid, password);

    BaseType_t xReturned;
    xReturned = xTaskCreate(SendData, "SendData", 10000, NULL, 1, &TaskHandle_1);
    if (xReturned == pdPASS) {
      Serial.println("Task Created");
    }
  }
}

void loop() {
}

void SendData(void *pvParameters) {
  while (1) {

    if (!cap) {
      GPSData gps_data;
      if (read_gps_data(gps_data)) {
        if (gps_data.lat != prev_lat || gps_data.lng != prev_lng || gps_data.speed != prev_speed) {
          String json = create_json(gps_data);
          SERIAL_PRINT_LN("Sending JSON: " + json);
          send_to_server(json);

          prev_lat = gps_data.lat;
          prev_lng = gps_data.lng;
          prev_speed = gps_data.speed;
        } else {
          SERIAL_PRINT_LN("No change in GPS data. Skipping send.");
        }
      }

    } else {
      Serial.println("ESP Restarted!!!!");
      preferences.putBool("captive", true);

      preferences.putString("ssid", "");
      preferences.putString("password", "");
      preferences.putString("endpoint", "");
      Serial.println(captive);
      delay(5000);
      ESP.restart();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay for 1 second
  }
}

void startCaptivePortal() {
  WiFi.softAP("ESP32-Config", "12345678");
  Serial.println(WiFi.softAPIP());

  // HTML content with CSS
  const char *captivePortalHTML = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      <title>ESP32 Configuration</title>
      <style>
        * {
          box-sizing: border-box;
          margin: 0;
          padding: 0;
        }

        body {
          font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
          background: linear-gradient(to right, #74ebd5, #ACB6E5);
          display: flex;
          justify-content: center;
          align-items: center;
          height: 100vh;
        }

        .container {
          background-color: #ffffff;
          padding: 30px 40px;
          border-radius: 12px;
          box-shadow: 0 8px 20px rgba(0, 0, 0, 0.2);
          max-width: 400px;
          width: 100%;
        }

        h2 {
          margin-bottom: 20px;
          color: #333;
        }

        label {
          display: block;
          text-align: left;
          margin-bottom: 5px;
          font-weight: 500;
        }

        input[type='text'],
        input[type='password'] {
          width: 100%;
          padding: 12px;
          margin-bottom: 15px;
          border: 1px solid #ccc;
          border-radius: 8px;
          font-size: 14px;
        }

        input[type='submit'] {
          width: 100%;
          background-color: #4CAF50;
          color: white;
          padding: 12px;
          border: none;
          border-radius: 8px;
          font-size: 16px;
          cursor: pointer;
          transition: background-color 0.3s ease;
        }

        input[type='submit']:hover {
          background-color: #45a049;
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>ESP32 Configuration</h2>
        <form action='/save' method='POST'>
          <label for='ssid'>SSID</label>
          <input type='text' id='ssid' name='ssid' placeholder='Enter SSID' required>

          <label for='password'>Password</label>
          <input type='password' id='password' name='password' placeholder='Enter Password' required>

          <label for='end-point'>Endpoint</label>
          <input type='text' id='end-point' name='end-point' placeholder='Enter endpoint' required>

          <input type='submit' value='Save'>
        </form>
      </div>
    </body>
    </html>
    )rawliteral";


  // Handle root URL
  server.on("/", HTTP_GET, [captivePortalHTML](AsyncWebServerRequest *request) {
    request->send(200, "text/html", captivePortalHTML);
  });

  // Handle form submission
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {

    if (request->hasParam("ssid", true)) {
      ssid = request->getParam("ssid", true)->value();
      Serial.println("Received SSID: " + ssid);
    }
    if (request->hasParam("password", true)) {
      password = request->getParam("password", true)->value();
      Serial.println("Received Password: " + password);
    }

    if (request->hasParam("end-point", true)) {
      end_point = request->getParam("end-point", true)->value();
      Serial.println("Received Password: " + password);
    }

    if (ssid.isEmpty() || password.isEmpty() || end_point.isEmpty()) {
      request->send(400, "text/html", "SSID or Password or endpoint cannot be empty.");
      return;
    }

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("endpoint", end_point);

    request->send(200, "text/html", "Credentials saved. Restarting...");
    delay(2000);
    ESP.restart();
  });

  // Start the server
  server.begin();
}
