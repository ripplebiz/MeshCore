#!/usr/bin/env python3
"""
Post-upload WiFi configuration script for MeshCore devices.
Automatically configures WiFi credentials after firmware upload.

Usage:
  python configure_wifi.py <ssid> <password> [port]
  
Examples:
  python configure_wifi.py "MyWiFi" "mypassword"
  python configure_wifi.py "MyWiFi" "mypassword" COM5
"""

import serial
import time
import sys
import os
import argparse
from serial.tools import list_ports

def find_device_port():
    """Find the COM port for the ESP32 device"""
    ports = list_ports.comports()
    for port in ports:
        if port.vid == 0x303A and port.pid == 0x1001:  # ESP32-S3
            return port.device
        if "USB Serial Device" in port.description and "303A:1001" in port.hwid:
            return port.device
    return None

def configure_wifi(port, ssid, password, timeout=30):
    """Configure WiFi on the device via serial CLI"""
    try:
        print(f"Connecting to {port}...")
        ser = serial.Serial(port, 115200, timeout=2)
        
        # Wait for device to boot
        print("Waiting for device to boot...")
        time.sleep(3)
        
        # Clear any pending data
        ser.reset_input_buffer()
        
        # Send WiFi configuration commands
        commands = [
            f"wifi set {ssid} {password}",
            "wifi enable",
            "wifi connect",
            "wifi status"
        ]
        
        for cmd in commands:
            print(f"Sending: {cmd}")
            ser.write((cmd + "\r").encode())
            time.sleep(1)
            
            # Read response
            response = ""
            start_time = time.time()
            while time.time() - start_time < 5:
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                    response += data
                time.sleep(0.1)
            
            print(f"Response: {response.strip()}")
            
        ser.close()
        print("✅ WiFi configuration completed!")
        return True
        
    except Exception as e:
        print(f"❌ Error configuring WiFi: {e}")
        return False

def main():
    """Main function called by PlatformIO or directly"""
    parser = argparse.ArgumentParser(
        description='Configure WiFi on MeshCore device',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python configure_wifi.py "MyWiFi" "mypassword"
  python configure_wifi.py "MyWiFi" "mypassword" COM5
        """
    )
    
    parser.add_argument('ssid', help='WiFi SSID (network name)')
    parser.add_argument('password', help='WiFi password')
    parser.add_argument('port', nargs='?', help='Serial port (auto-detect if not specified)')
    
    # Check if called by PlatformIO (no args) - fall back to env vars
    if len(sys.argv) == 1:
        # Called by PlatformIO - try environment variables for backward compatibility
        ssid = os.environ.get('WIFI_SSID')
        password = os.environ.get('WIFI_PWD')
        port = None
        
        if not ssid or not password:
            print("⚠️  No WiFi credentials provided.")
            print("   Usage: python configure_wifi.py <ssid> <password> [port]")
            print("   Or set WIFI_SSID and WIFI_PWD environment variables")
            return
    else:
        # Parse command line arguments
        args = parser.parse_args()
        ssid = args.ssid
        password = args.password
        port = args.port
    
    print(f"🔧 Configuring WiFi: {ssid}")
    
    # Find device port if not specified
    if not port:
        port = find_device_port()
        if not port:
            print("❌ Could not find ESP32 device port")
            return
    
    # Wait a moment for device to be ready after upload
    print("Waiting for device to be ready...")
    time.sleep(5)
    
    # Configure WiFi
    success = configure_wifi(port, ssid, password)
    
    if success:
        print("🎉 Device is ready with WiFi configured!")
    else:
        print("⚠️  WiFi configuration failed. You can configure manually via CLI.")

if __name__ == "__main__":
    main() 