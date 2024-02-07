
void sendJson() {
  Json_Doc["ID"] = DEVICE_ID;
  Json_Doc["co2"] = co2;
  Json_Doc["%RH"] = humidity;
  Json_Doc["RHSP"] = saved_parameters.rh.Setpoint; //only going to one decimal place
  Json_Doc["boxTempC"] = box_temperature;
  Json_Doc["BHSP"] = saved_parameters.bh.Setpoint; //only going to one decimal place
  Json_Doc["waterTempC"] = ih_input;
  Json_Doc["IHSP"] = saved_parameters.ih.Setpoint;
  serializeJson(Json_Doc, Serial);  // Generate the minified JSON and send it to the Serial port.
  Serial.println();
}


void readCO2() {
  co2 = scd41.getCO2();
}


void readHumidity() {
  if (sht.readSample()) humidity = sht.getHumidity();
  rh_input = humidity;
}


void readTemperature() {
  if (sht.readSample()) box_temperature = sht.getTemperature();
  bh_input = box_temperature;
}


void angleCalc() {
  angle = fmod((float)millis(), T) * 2 * PI / T + phase_shift;
  saved_parameters.phase_shift = angle;
  EEPROM.put(flash_address, saved_parameters); // Save the parameters to EEPROM
}


void ihSinusoidSetpoint() { 
  saved_parameters.ih.Setpoint = ih_a * sin(angle) + ih_b;
}

void bhSinusoidSetpoint() {
  saved_parameters.bh.Setpoint = bh_a * sin(angle) + bh_b;
}

void rhSinusoidSetpoint() {
  saved_parameters.rh.Setpoint = rh_a * sin(angle) + rh_b;
}


void readThermistor() {
  int raw_reading = analogRead(THERMISTOR_PIN);
  float v_ntc = float(raw_reading) * 5.0 / 1023;
  static const int R_FIXED = 10000;
  static const int V_DD = 5;
  float R_now = R_FIXED / (V_DD / v_ntc - 1);  // Calculate resistance of the NTC thermistor at the current temperature given the voltage measured by the ADC. This is if thermistor between GND and ADC pin.
  // Coefficients for [R,T]=[(0,), (20), (40), (60)] *ohms and °C
  static const double A = 0.001160448499734;  // constants A-D found using calculator on this website https://www.northstarsensors.com/calculating-temperature-from-resistance#:~:text=NTC%20(%25%2F%C2%B0C)%20%C3%97,%3D%20%C2%B1%200.586%20%25%20Resistance%20Tolerance.
  static const double B = 0.00022355684714;   // resistance and temperature values for calculator came from the thermistor's datasheet: https://uk.farnell.com/w/c/circuit-protection/thermistors/temperature-sensing-compensation-ntc-thermistors?brand=yageo&thermistor-mounting=free-hanging&range=inc-in-stock|exc-delivery-surcharge&sort=P_PRICE
  static const double C = 1.177892127367e-06;
  static const double D = 4.429960130453e-08;
  float T_kelvin = 1.0 / (A + B * log(R_now) + C * pow(log(R_now), 2) + D * pow(log(R_now), 3));  // 1/T = A+B*ln(R)+C*(ln(R))^2+D*(ln(r))^3
  static const float KELVIN_CONSTANT = 273.15;
  water_temperature = T_kelvin - KELVIN_CONSTANT; //return temperature in Celsius
  ih_input = water_temperature;
}


void displayValues() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("CO2=");
  display.print(co2);
  display.println("ppm");

  display.setCursor(0, 10);
  display.print("Temp=");
  display.print((int)box_temperature);
  display.println("C");

  display.setCursor(0, 20);
  display.print("RH=");
  display.print((int)humidity);
  display.println("%");
    
  if(digitalRead(WATER_LEVEL_PIN)) {
    display.setCursor(65, 0);
    display.println("WATER LOW!");
  }

  display.setCursor(65, 10);
  display.print("SetBox=");
  display.print((int)saved_parameters.bh.Setpoint);
  display.println("C");

  display.setCursor(71, 20);
  display.print("SetRH=");
  display.print((int)saved_parameters.rh.Setpoint);
  display.println("%");

  display.setCursor(40, 25);
  display.print("ID=");
  display.println(DEVICE_ID);
  
  display.display();
}

void handleUserInput() {
  String user_input = Serial.readString(); //Read user input string
  user_input.trim(); //Remove trailing whitespace characters
  char first_char = user_input.charAt(0); // Get the first character

  if (first_char == 'z') {
    phase_shift = 0; //NOTE!!! THIS DOES NOT RESET SETPOINT TO ZERO, JUST GETS RID OF PHASE SHIFT. THIS IS BECAUSE SINUSOIDAL SETPOINT IS A FUNCTION OF millis()! NEED TO IMPLEMENT A BUFFER OR SOMETHING IN angleCalc() IF YOU WANT THAT FUNCTIONALITY!!
    Serial.println("Phase Shift zeroed.");
  } else {
    Serial.println("Invalid input first char");
  }
  
  EEPROM.put(flash_address, saved_parameters); //save parameters to flash memory
}
