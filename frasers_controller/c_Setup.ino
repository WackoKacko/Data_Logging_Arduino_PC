
void setup() {
  //Pins
  pinMode(SOLENOID_VALVE_RELAY_PIN, OUTPUT);
  pinMode(BOX_HEATER_RELAY_PIN, OUTPUT);
  pinMode(IMMERSION_HEATER_RELAY_PIN, OUTPUT);
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(SOLENOID_VALVE_RELAY_PIN, LOW);
  digitalWrite(BOX_HEATER_RELAY_PIN, HIGH);
  digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH);

  wdt_enable(WDT_PERIOD_2KCLK_gc); //this is to enable watchdog so we can do a software reset every once in a while

  //Serial communication
  Serial.begin(115200);
  while(!Serial) {}

  wdt_reset(); //keeps watchdog from performing a software reset

  //I2C communication
  Wire.begin(); //begin I2C communication
  Serial.println(F("I2C up."));

  // SHT30 sensor start
  if (sht.init()) {
    Serial.println(F("SHT30 OK"));
    sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x
  }
  else Serial.println(F("SHT30 FAILED"));

  wdt_reset(); //keeps watchdog from performing a software reset

  //SCD41 sensor start
  if (scd41.begin(Wire) == true) {
    Serial.println("SCD41 OK"); //begin SCD41 sensor
    scd41.stopPeriodicMeasurement(); 
    delay(200);
    scd41.startPeriodicMeasurement();
  }

  wdt_reset(); //keeps watchdog from performing a software reset

  //BMP280 sensor start
  if (bmp.begin(0x76)) {
    Serial.println("BMP280 OK");
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  } else {
    delay(200);
    if (bmp.begin(0x76)) {
      Serial.println("BMP280 OK");
      bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                      Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                      Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                      Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                      Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
    } else Serial.println("BMP280 Failed!");
  }

  wdt_reset(); //keeps watchdog from performing a software reset

  //Little I2C Screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  else Serial.println(F("Screen ok!"));
  display.display(); // Display splash screen
  delay(500);
  display.clearDisplay();

  wdt_reset(); //keeps watchdog from performing a software reset

  //Task scheduler
  Runner.init();
  Runner.addTask(CheckCO2);
  Runner.addTask(CheckRH);
  Runner.addTask(CheckWaterTemp);
  Runner.addTask(CheckBoxTemp);
  Runner.addTask(CheckPressure);
  Runner.addTask(SendJson);
  Runner.addTask(AngleCalc);
  Runner.addTask(IhSinusoidSetpoint);
  Runner.addTask(BhSinusoidSetpoint);
  Runner.addTask(RhSinusoidSetpoint);
  Runner.addTask(DisplayValues);
  Runner.addTask(RequestTime);
  CheckCO2.enable();
  CheckRH.enable();
  CheckWaterTemp.enable();
  CheckBoxTemp.enable();
  CheckPressure.enable();
  SendJson.enable();
  AngleCalc.enable();
  IhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  BhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  RhSinusoidSetpoint.enable(); //ENABLE THESE IF YOU WISH TO TURN ON THE DAY/NIGHT SETPOINT SCHEDULING
  DisplayValues.enable();
  RequestTime.enable();
  Serial.println("Initialized scheduler");

  ih_start = millis();
  bh_start = millis();
  rh_start = millis();
 
  while(isnan(ih_input)) { readThermistor(); }
  ih_setpoint = ih_input;
  while(isnan(rh_input)) { readHumidity(); }
  rh_setpoint = rh_input;
  while(isnan(bh_input)) { readTemperature(); }
  bh_setpoint = bh_input;
  
  ih_PID.SetOutputLimits(0, WINDOW_SIZE); //tell the PID to range between 0 and the full window size
  ih_PID.SetMode(AUTOMATIC); //turn the PID on
  bh_PID.SetOutputLimits(0, WINDOW_SIZE); //tell the PID to range between 0 and the full window size
  bh_PID.SetMode(AUTOMATIC); //turn the PID on
  rh_PID.SetOutputLimits(0, WINDOW_SIZE); //tell the PID to range between 0 and the full window size
  rh_PID.SetMode(AUTOMATIC); //turn the PID on
}