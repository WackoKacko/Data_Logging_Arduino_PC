import serial
import time
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("serial_port")
parser.add_argument("baud_rate", type=int)
args = parser.parse_args()

try:
    with serial.Serial(args.serial_port, args.baud_rate, timeout=10) as ser:
        start_time = time.time()
        while time.time() - start_time < 10:  # Check for data within 10 seconds
            if ser.in_waiting > 0:
                data = ser.readline().decode().strip()
                print(f"Data received: {data}")
                exit(0)  # Exit successfully if data is received
        else:
            print("Timeout: No data received.")
            exit(1)
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit(1)
