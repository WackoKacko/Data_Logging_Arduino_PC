
//Sensors
SCD4x scd41;
AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR); //sensor address, sensor type

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

//Tasks
Task CheckCO2(10*1000, TASK_FOREVER, &readCO2); //check CO2 every 10 seconds
Task CheckRH(2*1000, TASK_FOREVER, &readHumidity); //check relative humidity every 2 seconds
Task CheckWaterTemp(2*1000, TASK_FOREVER, &readTemperature); //check water temperature every 2 seconds
Task CheckBoxTemp(2*1000, TASK_FOREVER, &readThermistor); //check box temperature every 2 seconds
Task SendJson(10*1000, TASK_FOREVER, &sendJson); //send box temp, co2, and relative humidity every 5 seconds
Task PlotSystem(100, TASK_FOREVER, &plotSystem);
Task PlotSystems(100, TASK_FOREVER, &plotSystems);


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

//I might need to declare all the below as "global" and do it in setup() once flash has read the inputs. I hope I don't get error from setpoint.
double ih_input, ih_output; // ("ih" stands for "immersion heater")
PID ih_PID(&ih_input, &ih_output, &(saved_parameters.ih.Setpoint), default_systems.ih.Kp, default_systems.ih.Ki, default_systems.ih.Kd, DIRECT);
unsigned long ih_start, ih_duration;

double bh_input, bh_output; // ("bh" stands for "box heater")
PID bh_PID(&bh_input, &bh_output, &(saved_parameters.bh.Setpoint), default_systems.bh.Kp, default_systems.bh.Ki, default_systems.bh.Kd, DIRECT);
unsigned long bh_start, bh_duration;

double rh_input, rh_output; // ("rh" stands for "relative humidity")
PID rh_PID(&rh_input, &rh_output, &(saved_parameters.rh.Setpoint), default_systems.rh.Kp, default_systems.rh.Ki, default_systems.rh.Kd, DIRECT);
unsigned long rh_start, rh_duration;

const unsigned int WINDOW_SIZE = 5000;

PID* system_plotted = &ih_PID; //edit this to plot other systems
