
void sendJson() {
  Json_Doc["co2"] = co2;
  Json_Doc["%RH"] = humidity;
  Json_Doc["tempC"] = box_temperature;
  serializeJson(Json_Doc, Serial);  // Generate the minified JSON and send it to the Serial port.
}


void readCO2() {
  co2 = scd41.getCO2();
  printf("CO2: %i\n", co2);
  // return co2;
}


void readHumidity() {
  humidity = aht10.readHumidity();
  rh_input = humidity;
  // printf("Humidity: %0.1f\n", humidity);
  // return humidity;
}


void readTemperature() {
  box_temperature = aht10.readTemperature();
  bh_input = box_temperature;
  // printf("Box temperature: %0.1f\n", bh_input);
  // printf("Box output: %0.1f\n", bh_output); //this gives null because P, I, and D are nan. Figure out how to make it so that all of them are set to 1 or something else.
  return box_temperature;
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
  // printf("Water temperature: %0.1f\n", water_temperature);
  ih_input = water_temperature;
}


void flashLED() {
  static unsigned long flash_time;
  if (millis()-flash_time > 50) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    flash_time = millis();
  }
}


void plotSystem() {
  Serial.print("Input:"); Serial.print(*(system_plotted->myInput)); Serial.print(",");
  Serial.print("Setpoint:"); Serial.print(*(system_plotted->mySetpoint)); Serial.print(",");
  Serial.print("Output:"); Serial.print(*(system_plotted->myOutput)/1000); //Serial.print(",");
  Serial.println();
}


void handleUserInput() { //more neat but I can't get printf() to work for %s type.
  String user_input = Serial.readString(); //Read user input string
  user_input.trim(); //Remove trailing whitespace characters
  Serial.print("Received: "); Serial.println(user_input);

  PID_params* actuator;//, actuator_flash;
  PID* process;
  char first_char = user_input.charAt(0); // Get the first character
  char actuator_name[20];
  // Serial.print("First char: "); Serial.println(first_char);
  if (first_char == 'i') {
    actuator = &(saved_parameters.ih);
    strcpy(actuator_name, "Immersion Heater");
    process = &(ih_PID);
  } else if (first_char == 'b') {
    actuator = &(saved_parameters.bh);
    strcpy(actuator_name, "Box Heater");
    process = &(bh_PID);
  } else if (first_char == 'r') {
    actuator = &(saved_parameters.rh);
    strcpy(actuator_name, "Relative Humidity");
    process = &(rh_PID);
  } else {
    Serial.println("Invalid input first char");
  }

  double* parameter;//, parameter_flash;
  char param_name[12];
  char second_char = user_input.charAt(1); // Get the second character
  if (second_char == 'p') {         //if user input p, we're dealing with Kp.
    parameter = &(actuator->Kp); 
    strcpy(param_name, "Kp");
  } else if (second_char == 'i') {  //if user input i, we're dealing with Ki.
    parameter = &(actuator->Ki);
    strcpy(param_name, "Ki");
  } else if (second_char == 'd') {  //if user input d, we're dealing with Kd.
    parameter = &(actuator->Kd); 
    strcpy(param_name, "Kd");
  } else if (second_char == 's') { //if user input s, we're dealing with setpoint
    parameter = &(actuator->Setpoint);
    strcpy(param_name, "Setpoint");
  } else {
    Serial.println("Invalid input second char");
  }
    
  String remaining_string = user_input.substring(2);  // Get the remaining part of the string
  if (remaining_string == "?") {
    printf("%s=%0.1f for %s\n", param_name, *parameter, actuator_name);
  } else if (remaining_string == "+") {
    *parameter += 0.1;
    printf("%s+=0.1, %s=%0.1f for %s\n", param_name, param_name, *parameter, actuator_name);
  } else if (remaining_string == "++") {
    *parameter += 1;
    printf("%s+=1, %s=%0.1f for %s\n", param_name, param_name, *parameter, actuator_name);
  } else if (remaining_string == "-") {
    if(*parameter>=0.1) *parameter -= 0.1;
    else Serial.println("That would give a negative number, which isn't allowed.");
    printf("%s-=0.1, %s=%0.1f for %s\n", param_name, param_name, *parameter, actuator_name);
  } else if (remaining_string == "--") {
    if(*parameter>=1) *parameter -= 1;
    else Serial.println("That would give a negative number, which isn't allowed.");
    printf("%s-=1, %s=%0.1f for %s\n", param_name, param_name, *parameter, actuator_name);
  } else if (remaining_string.charAt(0) == '=') {
    double number = remaining_string.substring(1).toDouble();
    if (number != 0.0) { // Conversion successfu.
      *parameter = number;
      printf("%s=%0.1f for %s\n", param_name, number, actuator_name);
    } else { // Conversion failed (invalid input)
      Serial.println("Invalid input after '='");
    }
  } else {
    Serial.println("Invalid rest of input");
  }

  process->SetTunings(actuator->Kp, actuator->Ki, actuator->Kd); //update PID system with new settings
  EEPROM.put(flash_address, saved_parameters); //save parameters to flash memory
}