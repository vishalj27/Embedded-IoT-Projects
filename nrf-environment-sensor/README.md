# IoT Sensor Monitoring System

A comprehensive IoT solution for monitoring temperature and humidity using BLE (Bluetooth Low Energy) communication and cloud data storage.

## System Architecture

```
┌─────────────────┐    BLE Advertising   ┌─────────────────┐    HTTP POST    ┌─────────────────┐
│   BLE Sensor    │ ──────────────────── │  Central Device │ ──────────────► │   Web Server    │
│   (E001)        │                      │   (ESP32)       │                 │  (FastAPI)      │
└─────────────────┘                      └─────────────────┘                 └─────────────────┘
                                                                                        │
                                                                                        ▼
                                                                               ┌─────────────────┐
                                                                               │   PostgreSQL    │
                                                                               │    Database     │
                                                                               └─────────────────┘
```

## Components

### 1. BLE Sensor Device (Bluefruit)
- **Device Name**: E001
- **Function**: Reads temperature and humidity data from HTU21D sensor
- **Communication**: Broadcasts data via BLE advertisements
- **Data Format**: Custom manufacturer-specific data packet

### 2. Central Device (ESP32)
- **Function**: Scans for BLE advertisements from target devices
- **Communication**: Receives BLE data and forwards to web server via HTTP POST
- **Connectivity**: WiFi enabled for internet communication

### 3. Web Server (FastAPI + PostgreSQL)
- **Backend**: FastAPI framework
- **Database**: PostgreSQL for data storage
- **Deployment**: Docker containerized setup

## Getting Started

### Prerequisites
- Arduino IDE with Bluefruit library support
- ESP32 development board
- HTU21D temperature/humidity sensor
- Docker and Docker Compose
- WiFi network credentials

### Hardware Setup

#### BLE Sensor Device
- Connect HTU21D sensor to Bluefruit board via I2C
- Power the device appropriately

#### Central Device (ESP32)
- Program ESP32 with the central device code
- Ensure WiFi connectivity

### Software Installation

#### 1. BLE Sensor Device Setup
```bash
# Install required libraries in Arduino IDE:
# - Adafruit Bluefruit nRF52 Libraries
# - SparkFun HTU21D Library
# - Adafruit SPIFlash Library
```

Upload the BLE sensor code to your Bluefruit device.

#### 2. Central Device Setup
```bash
# Install required libraries in Arduino IDE:
# - ESP32 BLE Arduino
# - WiFi library
# - HTTPClient library
```

**Configuration:**
```cpp
// Update these values in the central device code
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
String url = "http://YOUR_SERVER_IP:8000/api/sensor-data";
```

#### 3. Server Setup
```bash
cd Server

# Start the services using Docker Compose
docker-compose up --build
```

## Data Protocol

### BLE Advertisement Data Format
```
Custom Data Packet Structure:
-------------------------------------------------
| id_L | id_H | temp_L | temp_H | hum_L | hum_H |
-------------------------------------------------
```

- **ID**: Device identifier
- **Temperature**: Scaled by 100 (divide by 100 to get actual value)
- **Humidity**: Scaled by 100 (divide by 100 to get actual value)

### HTTP API Format
```json
{
  "deviceid": 1,
  "data": {
    "temperature": [{"temp": 25.30}],
    "humidity": [{"percentage": 65.20}]
  }
}
```

## Configuration

### BLE Sensor Configuration
```cpp
#define DEBUG 1              // Enable/disable debug output
#define SENSOR_DATA 0        // 0: Random data, 1: Actual sensor data
#define DEVICE_NAME "E001"   // Device identifier
```

### Central Device Configuration
```cpp
const String targetDevices[] = {"E001"};  // Target device names
const unsigned long scanInterval = 1000;  // Scan interval in milliseconds
```

### Server Configuration
```yaml
# docker-compose.yml
services:
  postgres:
    ports:
      - "5000:5432"  # Database port
  web:
    ports:
      - "8000:8000"  # API server port
```



## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/sensor-data` | Submit sensor data |
| GET | `/api/devices` | List registered devices |
| GET | `/api/data/{device_id}` | Get device data history |

---
