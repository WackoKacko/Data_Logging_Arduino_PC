
//Little I2C Screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x6
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//Sensors
SCD4x scd41;
// AHTxx aht(AHTXX_ADDRESS_X38, AHT1x_SENSOR); //sensor address, sensor type (aht10)
AHTxx aht(AHTXX_ADDRESS_X38, AHT2x_SENSOR); //(aht21)
SHTSensor sht;

//Global variables
float box_temperature, humidity, water_temperature;
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
void handleUserInput();
void displayValues();
void setTempRH();

//Tasks
Task CheckCO2(10*1000, TASK_FOREVER, &readCO2); //check CO2 every 10 seconds
Task CheckRH(1*1000, TASK_FOREVER, &readHumidity); //check relative humidity every 2 seconds
Task CheckWaterTemp(1*1000, TASK_FOREVER, &readTemperature); //check water temperature every 2 seconds
Task CheckBoxTemp(1*1000, TASK_FOREVER, &readThermistor); //check box temperature every 2 seconds
Task SendJson(10*1000, TASK_FOREVER, &sendJson); //send box temp, co2, and relative humidity every 5 seconds
Task SetTempRH(2*1000, TASK_FOREVER, &setTempRH); //copy AHT2X readings from outside to be setpoint for inside environment
Task DisplayValues(1*1000, TASK_FOREVER, &displayValues);

double ih_input, ih_output, ih_setpoint; // ("ih" stands for "immersion heater")
PID ih_PID(&ih_input, &ih_output, &ih_setpoint, 1000, 500, 100, DIRECT); //everything comes out initialized as zero.
unsigned long ih_start;

double bh_input, bh_output, bh_setpoint; // ("bh" stands for "box heater")
PID bh_PID(&bh_input, &bh_output, &bh_setpoint, 400, 400, 100, DIRECT);
unsigned long bh_start;

double rh_input, rh_output, rh_setpoint; // ("rh" stands for "relative humidity")
PID rh_PID(&rh_input, &rh_output, &rh_setpoint, 1000, 500, 100, DIRECT);
unsigned long rh_start, last_change;

const unsigned int MIN_CHANGE_TIME = 500;
const unsigned int WINDOW_SIZE = 3000; //for PID

const unsigned long WATCHDOG_TIMEOUT_PERIOD = 1.8e6; //30 minutes = 1.8e6 ms.
