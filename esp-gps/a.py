import io

import pynmea2
import serial
import requests

import json
from datetime import datetime

ser = serial.Serial('COM4', 9600, timeout=5.0)
sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))


gga_data = {}
rmc_data = {}

prev_lat = prev_lng = None
prev_speed = prev_alt = prev_sats = 0.0 

def create_json_payload(name, lat, lng, alt, speed, sats, mac, timestamp=None):
    # if timestamp is None:
    #     timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
    
    payload = {
        "name": name,
        "lat": round(lat, 6) if lat is not None else 0.0,
        "long": round(lng, 6) if lng is not None else 0.0,
        "timestamp": timestamp,
        "mac_address": mac,
        "sats": str(sats) if sats is not None else "0",
        "alt": round(alt, 2) if alt is not None else 0.0,
        "speed":round(speed, 2) if speed is not None else 0.0

    }
    return json.dumps(payload)

def send_to_server(json_payload):
    url = "https://10.185.151.167:3300/api/location/create"  # Replace with your server URL
    headers = {"Content-Type": "application/json"}
    
    try:
        response = requests.post(url, data=json_payload, headers=headers, verify=False)
        if response.status_code == 200:
            print("Data sent successfully!")
        else:
            print(f"Failed to send data: {response.status_code}")
    except requests.RequestException as e:
        print(f"Error sending data: {e}")



while True:
    try:
        line = sio.readline()
        msg = pynmea2.parse(line)

        lat = lng  = timestamp = None
        speed = sats = alt = 0.0        

        mac_address = "00:14:22:01:23:45"  # Placeholder MAC address
        name = "GPS Device"  # You can set this dynamically
        # print(repr(msg))

        if isinstance(msg, pynmea2.types.talker.GGA):
            gga_data = {
                "lat": msg.latitude if msg.latitude else None,
                "lng": msg.longitude if msg.longitude else None,
                "alt": float(msg.altitude) if msg.altitude else None,
                "sats": int(msg.num_sats) if msg.num_sats else 0
            }
 
        if isinstance(msg, pynmea2.types.talker.RMC):
            speed_knots = msg.spd_over_grnd  # Speed in knots
            speed = speed_knots * 0.514444  # Convert to m/s

            if msg.datestamp and msg.timestamp:
                dt = datetime.combine(msg.datestamp, msg.timestamp)
                timestamp = dt.strftime("%Y-%m-%dT%H:%M:%SZ")
            
            rmc_data = {
                "speed": speed,
                "timestamp": timestamp
            }

        if gga_data and rmc_data:

            lat = gga_data["lat"]
            lng = gga_data["lng"]
            alt = gga_data["alt"]
            sats = gga_data["sats"]
            speed = rmc_data["speed"]
            timestamp = rmc_data["timestamp"]

            if(lat != prev_lat or lng != prev_lng or speed != prev_speed or alt != prev_alt or sats != prev_sats):
                json_payload = create_json_payload(name, lat, lng, alt, speed, sats, mac_address, timestamp)
                print(json_payload)
                send_to_server(json_payload)

                prev_lat = lat
                prev_lng = lng
                prev_speed = speed
                prev_alt = alt
                prev_sats = sats

                gga_data.clear()
                rmc_data.clear()


    except serial.SerialException as e:
        print('Device error: {}'.format(e))
        break
    except pynmea2.ParseError as e:
        print('Parse error: {}'.format(e))
        continue