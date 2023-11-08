
void setup() {

  //Serial communication
  Serial.begin(115200);
  while(!Serial) {}


  //I2C communication
  Wire.begin(); //begin I2C communication


  //AHT10 sensor start
  while (aht.begin() != true) { //begin AHT10 sensor
    Serial.println("AHT1x not connected or fail to load calibration coefficient");
    delay(5000);
  }
  Serial.println("AHT OK");
  aht.softReset();
  delay(200);


  //SCD41 sensor start
  if (scd41.begin(Wire) == true) Serial.println("SCD41 OK"); //begin SCD41 sensor
  scd41.stopPeriodicMeasurement(); delay(200);
  scd41.startPeriodicMeasurement();


  //Pins
  pinMode(SOLENOID_VALVE_RELAY_PIN, OUTPUT);
  pinMode(BOX_HEATER_RELAY_PIN, OUTPUT);
  pinMode(IMMERSION_HEATER_RELAY_PIN, OUTPUT);
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);


  //Task scheduler
  Runner.init();
  Runner.addTask(CheckCO2);
  Runner.addTask(CheckRH);
  Runner.addTask(CheckWaterTemp);
  Runner.addTask(CheckBoxTemp);
  Runner.addTask(SendJson);
  Runner.addTask(PlotSystem);
  Runner.addTask(PlotSystems);
  Runner.addTask(IhSinusoidSetpoint);
  Runner.addTask(BhSinusoidSetpoint);
  Runner.addTask(RhSinusoidSetpoint);
  CheckCO2.enable();
  CheckRH.enable();
  CheckWaterTemp.enable();
  CheckBoxTemp.enable();
  // SendJson.enable();
  // PlotSystem.enable();
  PlotSystems.disable();
  IhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  BhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  RhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  Serial.println("Initialized scheduler");


  //PID
  EEPROM.get(flash_address, saved_parameters); //Check if any of parameters are "nan"
  if (isnan(saved_parameters.ih.Kp)) saved_parameters.ih.Kp = 0;
  if (isnan(saved_parameters.ih.Ki)) saved_parameters.ih.Ki = 0;
  if (isnan(saved_parameters.ih.Kd)) saved_parameters.ih.Kd = 0;
  if (isnan(saved_parameters.ih.Setpoint)) saved_parameters.ih.Setpoint = 0;
  if (isnan(saved_parameters.bh.Kp)) saved_parameters.bh.Kp = 0;
  if (isnan(saved_parameters.bh.Ki)) saved_parameters.bh.Ki = 0;
  if (isnan(saved_parameters.bh.Kd)) saved_parameters.bh.Kd = 0;
  if (isnan(saved_parameters.bh.Setpoint)) saved_parameters.bh.Setpoint = 0;
  if (isnan(saved_parameters.rh.Kp)) saved_parameters.rh.Kp = 0;
  if (isnan(saved_parameters.rh.Ki)) saved_parameters.rh.Ki = 0;
  if (isnan(saved_parameters.rh.Kd)) saved_parameters.rh.Kd = 0;
  if (isnan(saved_parameters.rh.Setpoint)) saved_parameters.rh.Setpoint = 0;
  EEPROM.put(flash_address, saved_parameters); // Save the parameters to EEPROM
  ih_PID.SetTunings(saved_parameters.ih.Kp, saved_parameters.ih.Ki, saved_parameters.ih.Kd); //update PID system with new settings
  bh_PID.SetTunings(saved_parameters.bh.Kp, saved_parameters.bh.Ki, saved_parameters.bh.Kd); //update PID system with new settings
  rh_PID.SetTunings(saved_parameters.rh.Kp, saved_parameters.rh.Ki, saved_parameters.rh.Kd); //update PID system with new settings
  // ^ PID setpoints get updated automatically. Kp, Ki, and Kd don't.


  ih_start = millis();
  bh_start = millis();
  rh_start = millis();
 
  ih_input = 100; //higher than anything we'd expect so that nothing happens until first sensor readings
  bh_input = 100;
  rh_input = 100;
  
  ih_PID.SetOutputLimits(0, WINDOW_SIZE); //tell the PID to range between 0 and the full window size
  ih_PID.SetMode(AUTOMATIC); //turn the PID on
  bh_PID.SetOutputLimits(0, WINDOW_SIZE); //tell the PID to range between 0 and the full window size
  bh_PID.SetMode(AUTOMATIC); //turn the PID on
  rh_PID.SetOutputLimits(0, WINDOW_SIZE); //tell the PID to range between 0 and the full window size
  rh_PID.SetMode(AUTOMATIC); //turn the PID on
}