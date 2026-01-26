#!/usr/bin/env python3
"""
Serial to Server Bridge
Reads GPS data from Wio Terminal via USB Serial and sends to localhost server

Usage:
1. Upload wio_gps_serial.ino to Wio Terminal
2. Run: python3 gps_server.py (in another terminal)
3. Run: python3 serial_to_server.py
4. Open: http://localhost:5002
"""

import serial
import serial.tools.list_ports
import requests
import time
import sys

SERVER_URL = "http://localhost:5002/api/gps/update"

def find_wio_port():
    """Find the Wio Terminal serial port"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Wio Terminal usually shows as "usbmodem" on Mac or "USB" on Windows
        if 'usbmodem' in port.device.lower() or 'usb' in port.device.lower() or 'wio' in port.description.lower():
            return port.device

    # If not found, list all ports
    print("Available ports:")
    for port in ports:
        print(f"  {port.device} - {port.description}")

    return None

def main():
    # Find or specify port
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = find_wio_port()
        if not port:
            print("Wio Terminal not found. Specify port manually:")
            print("  python3 serial_to_server.py /dev/cu.usbmodemXXXX")
            return

    print(f"Connecting to {port}...")

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"Connected to {port}")
        print(f"Sending data to {SERVER_URL}")
        print("=" * 50)
        print("Waiting for GPS data...")
        print("(Make sure Wio Terminal is running wio_gps_serial.ino)")
        print("=" * 50)

        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            if line.startswith("GPS:"):
                # Parse: GPS:lat,lon,speed,altitude,satellites
                try:
                    data = line[4:].split(',')
                    lat = float(data[0])
                    lon = float(data[1])
                    speed = float(data[2])
                    altitude = float(data[3])
                    satellites = int(data[4])

                    # Send to server
                    params = {
                        'lat': lat,
                        'lon': lon,
                        'speed': speed,
                        'altitude': altitude,
                        'satellites': satellites
                    }

                    try:
                        requests.get(SERVER_URL, params=params, timeout=2)
                        print(f"Speed: {speed:.1f} MPH | Sat: {satellites} | ({lat:.5f}, {lon:.5f})")
                    except requests.exceptions.RequestException:
                        print("Server not running? Start: python3 gps_server.py")

                except (ValueError, IndexError) as e:
                    pass  # Ignore malformed data

            elif line:
                # Print other serial output for debugging
                print(f"[Serial] {line}")

    except serial.SerialException as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nStopped.")

if __name__ == '__main__':
    main()
