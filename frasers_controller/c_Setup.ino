
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
    // for(;;);
  }
  else Serial.println(F("Screen ok!"));
  display.display(); // Display splash screen
  delay(500);
  display.clearDisplay();


  // //AHT2X sensor start
  while (aht.begin() != true) { //begin AHT2X sensor
    Serial.println("AHT2X not connected or fail to load calibration coefficient");
    delay(1000);
  }
  Serial.println("AHT2X OK");
  aht.softReset();
  delay(200);


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
  Runner.addTask(SetTempRH);
  Runner.addTask(DisplayValues);
  CheckCO2.enable();
  CheckRH.enable();
  CheckWaterTemp.enable();
  CheckBoxTemp.enable();
  SendJson.enable();
  SetTempRH.enable();
  DisplayValues.enable();
  Serial.println("Initialized scheduler");

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