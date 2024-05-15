
//Little I2C Screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x6
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//Sensors
SCD4x scd41;
SHTSensor sht;
Adafruit_BMP280 bmp;


//Global variables
float box_temperature, humidity, water_temperature, pressure;
int co2;

//Json document for sending sensor data.
StaticJsonDocument<200> Json_Doc;

// Task scheduler
Scheduler Runner;

// Function prototypes
void sendJson();
void readCO2();
void readHumidity();
void readTemperature();
void readThermistor();
void readPressure();
void angleCalc();
void ihSinusoidSetpoint();
void bhSinusoidSetpoint();
void rhSinusoidSetpoint();
void displayValues();
void requestTime();

//Tasks
Task CheckCO2(10*1000, TASK_FOREVER, &readCO2); //check CO2 every 10 seconds
Task CheckRH(1*1000, TASK_FOREVER, &readHumidity); //check relative humidity every 2 seconds
Task CheckWaterTemp(1*1000, TASK_FOREVER, &readTemperature); //check water temperature every 2 seconds
Task CheckBoxTemp(1*1000, TASK_FOREVER, &readThermistor); //check box temperature every 2 seconds
Task CheckPressure(1*1000, TASK_FOREVER, &readPressure);
Task SendJson(10*1000, TASK_FOREVER, &sendJson); //send box temp, co2, and relative humidity every 5 seconds
Task AngleCalc(1*1000, TASK_FOREVER, &angleCalc);
Task IhSinusoidSetpoint(1*1000, TASK_FOREVER, &ihSinusoidSetpoint);
Task BhSinusoidSetpoint(1*1000, TASK_FOREVER, &bhSinusoidSetpoint);
Task RhSinusoidSetpoint(1*1000, TASK_FOREVER, &rhSinusoidSetpoint);
Task DisplayValues(1*1000, TASK_FOREVER, &displayValues);
Task RequestTime(500, TASK_FOREVER, &requestTime);

double ih_setpoint, bh_setpoint, rh_setpoint;

double ih_input = NAN;
double ih_output;  // ("ih" stands for "immersion heater")
PID ih_PID(&ih_input, &ih_output, &ih_setpoint, 1000, 500, 100, DIRECT);
unsigned long ih_start;

double bh_input = NAN;
double bh_output; // ("bh" stands for "box heater")
PID bh_PID(&bh_input, &bh_output, &bh_setpoint, 600, 500, 100, DIRECT);
unsigned long bh_start;

double rh_input = NAN;
double rh_output; // ("rh" stands for "relative humidity")
PID rh_PID(&rh_input, &rh_output, &rh_setpoint, 1000, 500, 100, DIRECT);
unsigned long rh_start;

const unsigned int MIN_CHANGE_TIME = 500;
const unsigned int WINDOW_SIZE = 3000; //for PID

const unsigned long T = 8.64e7; //Period in milliseconds. 1 day = 8.64e7 ms. ***WARNING!!! DO NOT PERFORM A CALCULATION HERE LIKE "T = 1000*60*60*24, THAT BREAKS THE CODE FOR ARCANE REASONS. INPUT THE EXACT NUMBER YOU WANT, PERHAPS IN SCIENTIFIC NOTATION.
float angle;
float start_time = NAN;

float ih_a = (IH_MAX - IH_MIN) / 2; float ih_b = (IH_MAX + IH_MIN) / 2;
float bh_a = (BH_MAX - BH_MIN) / 2; float bh_b = (BH_MAX + BH_MIN) / 2;
float rh_a = (RH_MAX - RH_MIN) / 2; float rh_b = (RH_MAX + RH_MIN) / 2;

const unsigned long WATCHDOG_TIMEOUT_PERIOD = 5.4e6; //30 minutes = 1.8e6 ms.
