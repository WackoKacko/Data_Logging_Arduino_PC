
//Sensors
SCD4x scd41;
// AHTxx aht(AHTXX_ADDRESS_X38, AHT1x_SENSOR); //sensor address, sensor type (aht10)
AHTxx aht(AHTXX_ADDRESS_X38, AHT2x_SENSOR); //(aht21)

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
void plotSystem();
void plotSystems();
void ihSinusoidSetpoint();
void bhSinusoidSetpoint();
void rhSinusoidSetpoint();

//Tasks
Task CheckCO2(10*1000, TASK_FOREVER, &readCO2); //check CO2 every 10 seconds
Task CheckRH(1*1000, TASK_FOREVER, &readHumidity); //check relative humidity every 2 seconds
Task CheckWaterTemp(1*1000, TASK_FOREVER, &readTemperature); //check water temperature every 2 seconds
Task CheckBoxTemp(1*1000, TASK_FOREVER, &readThermistor); //check box temperature every 2 seconds
Task SendJson(10*1000, TASK_FOREVER, &sendJson); //send box temp, co2, and relative humidity every 5 seconds
Task PlotSystem(200, TASK_FOREVER, &plotSystem);
Task PlotSystems(200, TASK_FOREVER, &plotSystems);
Task IhSinusoidSetpoint(1*1000, TASK_FOREVER, &ihSinusoidSetpoint);
Task BhSinusoidSetpoint(1*1000, TASK_FOREVER, &bhSinusoidSetpoint);
Task RhSinusoidSetpoint(1*1000, TASK_FOREVER, &rhSinusoidSetpoint);


//PID
typedef struct { // Define PID parameters to store in memory
  double Kp;
  double Ki;
  double Kd;
  double Setpoint;
} PID_params;

typedef struct { // Group PID parameters by system
  PID_params ih;
  PID_params bh;
  PID_params rh;
} PID_systems;

PID_params default_params = { // Define default PID parameters (Kp, Ki, Kd, Setpoint)
  1.0, 1.0, 1.0, 1.0,
};

PID_systems default_systems = {
  default_params, // ih
  default_params, // bh
  default_params  // rh
};

PID_systems saved_parameters; // Create global object to store PID settings in flash
int flash_address = 0; // EEPROM address where PID parameters are stored

double ih_input, ih_output; // ("ih" stands for "immersion heater")
PID ih_PID(&ih_input, &ih_output, &(saved_parameters.ih.Setpoint), default_systems.ih.Kp, default_systems.ih.Ki, default_systems.ih.Kd, DIRECT);
unsigned long ih_start, ih_duration;

double bh_input, bh_output; // ("bh" stands for "box heater")
PID bh_PID(&bh_input, &bh_output, &(saved_parameters.bh.Setpoint), default_systems.bh.Kp, default_systems.bh.Ki, default_systems.bh.Kd, DIRECT);
unsigned long bh_start, bh_duration;

double rh_input, rh_output; // ("rh" stands for "relative humidity")
PID rh_PID(&rh_input, &rh_output, &(saved_parameters.rh.Setpoint), default_systems.rh.Kp, default_systems.rh.Ki, default_systems.rh.Kd, DIRECT);
unsigned long rh_start, rh_duration;

const unsigned int MIN_WINDOW = 500;
const unsigned int WINDOW_SIZE = 3000; //for PID

const float IH_MAX = 25, IH_MIN = 25; //max and min water temperature
const float BH_MAX = 25, BH_MIN = 25; //max and min box temperature
const float RH_MAX = 80, RH_MIN = 50; //max and min relative humidity
// const unsigned long T = 1000*60*60*24; // period is one day (milliseconds in a day, =8.64e7)
unsigned long T = 1000*60*30; //period is 6 hours (milliseconds in 10 minutes)


PID* system_plotted = &ih_PID; //edit this to plot other systems

//NOTE: ALL OF THE BELOW FUNCTIONS START AT MIDPOINT
void ihSinusoidSetpoint() {
  unsigned long x=millis();
  // Serial.print("ih_time in seconds = "); Serial.println(x/1000);
  float a = (IH_MAX-IH_MIN)/2; //wave amplitude
  float b = (IH_MAX+IH_MIN)/2; //wave vertical offset
  saved_parameters.ih.Setpoint = a*sin(2*3.1416/T*x)+b;
}

void bhSinusoidSetpoint() {
  unsigned long x=millis();
  // Serial.print("bh_time in seconds = "); Serial.println(x/1000);
  float a = (BH_MAX-BH_MIN)/2; //wave amplitude
  float b = (BH_MAX+BH_MIN)/2; //wave vertical offset
  saved_parameters.bh.Setpoint = a*sin(2*3.1416/T*x)+b;
}

void rhSinusoidSetpoint() {
  unsigned long x=millis();
  // Serial.print("rh_time in seconds = "); Serial.println(x/1000);
  float a = (RH_MAX-RH_MIN)/2; //wave amplitude
  float b = (RH_MAX+RH_MIN)/2; //wave vertical offset
  saved_parameters.rh.Setpoint = a*sin(2*3.1416/T*x)+b;
}
