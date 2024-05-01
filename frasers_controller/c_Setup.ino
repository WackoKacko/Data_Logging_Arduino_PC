
void setup() {

  //Serial communication
  Serial.begin(115200);
  while(!Serial) {}

  Serial.println("Serial up. Initializing.");


  //I2C communication
  Wire.begin(); //begin I2C communication
  Serial.println(F("I2C up."));

  //Little I2C Screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  else Serial.println(F("Screen ok!"));
  display.display(); // Display splash screen
  delay(500);
  display.clearDisplay();


  // SHT30 sensor start
  if (sht.init()) Serial.println(F("SHT30 OK"));
  else Serial.println(F("SHT30 FAILED"));
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x


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
  Runner.addTask(AngleCalc);
  Runner.addTask(IhSinusoidSetpoint);
  Runner.addTask(BhSinusoidSetpoint);
  Runner.addTask(RhSinusoidSetpoint);
  Runner.addTask(DisplayValues);
  CheckCO2.enable();
  CheckRH.enable();
  CheckWaterTemp.enable();
  CheckBoxTemp.enable();
  SendJson.enable();
  AngleCalc.enable();
  IhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  BhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  RhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  DisplayValues.enable();
  Serial.println("Initialized scheduler");


  //PID
  EEPROM.get(flash_address, saved_parameters); //Check if any of parameters are "nan"
  if (isnan(saved_parameters.ih.Setpoint)) saved_parameters.ih.Setpoint = (IH_MAX + IH_MIN)/2;
  if (isnan(saved_parameters.bh.Setpoint)) saved_parameters.bh.Setpoint = (BH_MAX + BH_MIN)/2;
  if (isnan(saved_parameters.rh.Setpoint)) saved_parameters.rh.Setpoint = (RH_MAX + RH_MIN)/2;
  if (isnan(saved_parameters.phase_shift)) saved_parameters.phase_shift = 0;
  else phase_shift = saved_parameters.phase_shift;
  EEPROM.put(flash_address, saved_parameters); // Save the parameters to EEPROM

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

  wdt_enable(WDT_PERIOD_2KCLK_gc); //this is to enable watchdog so we can do a software reset every once in a while
}