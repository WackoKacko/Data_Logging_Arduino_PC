// Project Overview:
// This is a program for controlling the temperature and humidity inside a box. 
// There are 3 relays. Two are for heaters, one is for a solenoid valve.
// The heater relays do PID with phase angle control. 
// One heater is an immersion heater, the other a heating wire.
// The solenoid valve relay does PID with bang bang control (only on/off)
// There is an AHT10 sensor for measuring relative humidity (%RH) and temperature (°C).
// The temperature reading from the AHT10 is for the heating wire.
// The humidity reading from the AHT10 is for the solenoid valve.
// There is a thermistor. I use an external ADS1115 ADC to read it because I don't trust the ESP32's ADC. External ADC not used for the Arduino Nano Every.
// The temperature reading from the thermistor is for the immersion heater.
// There is a CO2 sensor inside the box, the SCD41. 
// The SCD41's temperature accuracy is only ±1.5°C, and its humidity accuracy is only ±9%RH. 
// The AHT10's temperature accuracy is ±0.3°C, and its humidity accuracy is ±2%RH. Much better than the SCD41.
// For the AHT10, note that prolonged exposure for 60 hours at humidity > 80% can lead to a temporary drift of the signal +3%. Sensor slowly returns to the calibrated state at normal operating conditions.
// The SCD41's CO2 accuracy is ±40ppm (+5% of reading), so true 500ppm could read 437-567ppm, I understand?
// The program communicates the most recent SCD41 and AHT10 readings every 10 seconds.


//The following libraries are used...
#include <Wire.h>               //built-in
#include <math.h>               //built-in
#include <ArduinoJson.h>        //built-in (https://github.com/bblanchon/ArduinoJson) <- I think that's the correct repo
#include "PID_v1.h"             //https://github.com/br3ttb/Arduino-PID-Library/tree/master
#include "TaskScheduler.h"      //https://github.com/arkhipenko/TaskScheduler/tree/master
#include "SHTSensor.h"      //???
#include "SparkFun_SCD4x_Arduino_Library.h"  //https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library/tree/main
#include <LibPrintf.h>            // https://github.com/embeddedartistry/arduino-printf
#include <EEPROM.h>               // built-in (https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/EEPROM)
#include <Watchdog.h>              //installed from library manager. https://www.arduino.cc/reference/en/libraries/watchdog/
#include <Adafruit_GFX.h> //installed from library manager, idk
#include <Adafruit_SSD1306.h> //installed from library manager, idk

// The following pins are used... (for the Arduino Nano Every)
#define SOLENOID_VALVE_RELAY_PIN 3
#define BOX_HEATER_RELAY_PIN 5
#define IMMERSION_HEATER_RELAY_PIN 6
#define THERMISTOR_PIN 14 //(A0)
#define WATER_LEVEL_PIN 21 //(A7)

const unsigned int DEVICE_ID = 5;
