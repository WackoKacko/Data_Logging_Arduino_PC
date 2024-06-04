
void loop() {
  Runner.execute(); //handles sensor readings and sending data over Serial

  wdt_reset(); //keeps watchdog from performing a software reset

  if ( millis() > WATCHDOG_TIMEOUT_PERIOD ) delay(5000); //software reset of Arduino using watchdog timer

  if(millis() - last_PID > 500) {
    rh_PID.Compute();
    if (millis() - rh_start > WINDOW_SIZE) rh_start = millis();// WINDOW_SIZE; //time to shift the Relay Window
    if (rh_output > millis() - rh_start && rh_output > MIN_CHANGE_TIME) digitalWrite(SOLENOID_VALVE_RELAY_PIN, HIGH); //NOTE THE >/<
    else digitalWrite(SOLENOID_VALVE_RELAY_PIN, LOW);

    if(!sht.readSample() || (box_temperature == 0 && humidity == 0) || box_temperature > 40) {
      // Serial.println("SHT sensor failure!"); //DO NOT UNCOMMENT THIS!!!
      digitalWrite(BOX_HEATER_RELAY_PIN, HIGH); 
    } else {
      bh_PID.Compute();
      if (millis() - bh_start > WINDOW_SIZE) bh_start = millis(); //time to shift the Relay Window
      if (bh_output < millis() - bh_start) digitalWrite(BOX_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
      else digitalWrite(BOX_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)
    }
    
    if(water_temperature < -5 || water_temperature > 40) {
      // Serial.println("Water temperature suspiciously low!"); //DO NOT UNCOMMENT THIS!!!
      digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH);
    } else {
      ih_PID.Compute();
      if (millis() - ih_start > WINDOW_SIZE) ih_start = millis(); //time to shift the Relay Window (ih_start = millis() also works)
      if (ih_output < millis() - ih_start) digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR). 
      else digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)
    }
  }
}