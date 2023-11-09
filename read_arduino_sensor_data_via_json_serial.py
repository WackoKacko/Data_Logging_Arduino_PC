import serial
import json
from openpyxl import Workbook
from openpyxl import load_workbook
from datetime import datetime

# Define the COM port for Arduino (e.g., COM9)
COM_PORT = 'COM4'

# Define the path to your Excel file
excel_file_path = r'C:\Users\bushb\Documents\CO2-RH-TEMPC.xlsx'

# Initialize the serial connection
try:
    ser = serial.Serial(COM_PORT, 115200, timeout=1)
    print(f"Connected to {COM_PORT}")
except Exception as e:
    print(f"Failed to connect to {COM_PORT}: {str(e)}")
    exit(1)

# Create or load the Excel workbook
try:
    workbook = load_workbook(excel_file_path)
except FileNotFoundError: # If no file exists, create one
    workbook = Workbook()
    workbook.save(excel_file_path)
    print(f"Created a new Excel file: {excel_file_path}")    
except Exception as e:
    print(f"Error opening Excel file: {str(e)}")
    exit(1)

# Select the default sheet (you can change the sheet name as needed)
sheet = workbook.active
sheet.append(["Time", "CO2 (ppm)", "%RH", "Celsius", "Relative Humidity Setpoint (%RH)", "Box Temp. Setpoint (°C)", "Water Temp. Setpoint (°C)"]) # Add labels to the first row

while True:
    # Read a line from the Arduino
    line = ser.readline().decode('utf-8').strip()

    try:
        sensor_data = json.loads(line)

        # Extract data from the JSON object
        co2 = sensor_data["co2"]
        humidity = sensor_data["%RH"]
        temperature = sensor_data["tempC"]
        rhsp = sensor_data["RHSP"]
        bhsp = sensor_data["BHSP"]
        ihsp = sensor_data["IHSP"]

        # Get the current date and time
        current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        # Append data to the Excel sheet
        sheet.append([current_time, co2, humidity, temperature, rhsp, bhsp, ihsp])

        # Save the Excel file
        workbook.save(excel_file_path)

        print(f"Data recorded: {current_time}, CO2: {co2}, RH: {humidity}, Temperature: {temperature}, RHSP: {rhsp}, BHSP: {bhsp}, IHSP: {ihsp}")
    except json.JSONDecodeError:
        print(f"Invalid JSON data: {line}")
    except KeyError as e:
        print(f"Missing key in JSON data: {e}")
    except Exception as e:
        print(f"Error processing data: {str(e)}")

# Close the serial connection when done
ser.close()
