
void sendJson() {
  Json_Doc["ID"] = DEVICE_ID;
  Json_Doc["co2"] = co2;
  Json_Doc["%RH"] = humidity;
  Json_Doc["RHSP"] = rh_setpoint; //only going to one decimal place
  Json_Doc["boxTempC"] = box_temperature;
  Json_Doc["BHSP"] = bh_setpoint; //only going to one decimal place
  Json_Doc["waterTempC"] = ih_input;
  Json_Doc["IHSP"] = ih_setpoint;
  Json_Doc["pressure"] = pressure; //pressure is a float
  serializeJson(Json_Doc, Serial);  // Generate the minified JSON and send it to the Serial port.
  Serial.println();
}


void readCO2() {
  scd41.setAmbientPressure(pressure);
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


void readPressure() { //returns pressure in millibar (mBar)
  pressure = bmp.readPressure();
}


void iso8601ToSeconds(String isoTime) {  // Calculate seconds since midnight
  long hour = isoTime.substring(11, 13).toInt();
  int minute = isoTime.substring(14, 16).toInt();
  int second = isoTime.substring(17, 19).toInt();

  start_time = (float)((hour * 3600) + (minute * 60) + second);
}


void angleCalc() {
  if(!isnan(start_time)) {
    angle = 2*PI/86400*(millis()/1000 + start_time - 50400);
  }
}


void ihSinusoidSetpoint() { 
  if(!isnan(start_time)) {
    ih_setpoint = ih_a * sin(angle+1.5708) + ih_b;
  }
}

void bhSinusoidSetpoint() {
  if(!isnan(start_time)) {
    bh_setpoint = bh_a * sin(angle+1.5708) + bh_b;
  }
}

void rhSinusoidSetpoint() {
  if(!isnan(start_time)) {
    rh_setpoint = rh_a * sin(angle+4.7124) + rh_b; 
  }
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
  display.print((int)bh_setpoint);
  display.println("C");

  display.setCursor(71, 20);
  display.print("SetRH=");
  display.print((int)rh_setpoint);
  display.println("%");

  display.setCursor(40, 25);
  display.print("ID=");
  display.println(DEVICE_ID);
  
  display.display();
}


void requestTime() {
  Serial.println("Serial up. Initializing.");
  // delay(10);
  if (Serial.available() > 0) {
      String iso_time = Serial.readStringUntil('\n');
      iso8601ToSeconds(iso_time);
      if(!isnan(start_time)) {
          RequestTime.disable();
          Serial.print("Received time: ");
          Serial.println(iso_time);
      } else {
          Serial.println("Still waiting for time sync.");
      }
  }
}
