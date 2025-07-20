import io
import pynmea2
import serial
import requests
import json
from datetime import datetime

# Replace with your actual server URL
SERVER_URL = "https://10.185.151.167:3300/api/location/create"
DEVICE_NAME = "GPS_Device_2"
MAC_ADDRESS = "AA:BB:CC:DD:EE:FF"  # Replace with actual MAC or generate it

# Set the serial port and baud rate according to your GPS module
ser = serial.Serial('COM4', 9600, timeout=5.0)
sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))

def create_json_payload(name, lat, lng, alt, speed, sats, mac, timestamp=None):
    if timestamp is None:
        timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
    
    payload = {
        "name": name,
        "lat": round(lat, 6),
        "long": round(lng, 6),
        "timestamp": timestamp,
        "mac_address": mac,
        "sats": str(sats),
        "alt": round(alt, 2),
        "speed": round(speed, 2)
    }
    return json.dumps(payload)

def send_to_server(json_data):
    try:
        response = requests.post(SERVER_URL, headers={"Content-Type": "application/json"}, data=json_data)
        print(f"HTTP Status: {response.status_code}")
        print("Server response:", response.text)
    except Exception as e:
        print("Failed to send data:", e)

while True:
    try:
        line = sio.readline().strip()
        if line.startswith('$'):
            msg = pynmea2.parse(line)

            if hasattr(msg, 'latitude') and hasattr(msg, 'longitude'):
                lat = msg.latitude
                lng = msg.longitude

                gps_date = getattr(msg, 'datestamp', None)  # datetime.date
                gps_time = getattr(msg, 'timestamp', None)  # datetime.time
                alt = getattr(msg, 'altitude', 0.0)
                speed = getattr(msg, 'spd_over_grnd', 0.0)
                sats = getattr(msg, 'num_sats', 0)

                # Build ISO 8601 timestamp from GPS date and time
                if gps_date and gps_time:
                    gps_datetime = datetime.combine(gps_date, gps_time)
                    gps_iso_timestamp = gps_datetime.strftime("%Y-%m-%dT%H:%M:%SZ")
                else:
                    gps_iso_timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")  # fallback

                json_payload = create_json_payload(
                    DEVICE_NAME, lat, lng, alt, speed, sats, MAC_ADDRESS, gps_iso_timestamp
                )

                print("Sending JSON:", json_payload)
                # Uncomment below to actually send
                # send_to_server(json_payload)

    except serial.SerialException as e:
        print('Device error:', e)
        break
    except pynmea2.ParseError as e:
        print('Parse error:', e)
        continue
