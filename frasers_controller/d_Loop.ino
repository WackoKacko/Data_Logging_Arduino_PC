
void loop() {
  Runner.execute(); //handles sensor readings and sending data over Serial

  wdt_reset(); //keeps watchdog from performing a software reset
  if (Serial.available() > 0) handleUserInput();

  if ( millis() > WATCHDOG_TIMEOUT_PERIOD ) delay(5000); //software reset of Arduino using watchdog timer

 //*NOTE: trying inverting "digitalWrite(..., HIGH)" and "digitalWrite(..., LOW)" if you are seeing unexpected behavior.
  rh_PID.Compute();
  if (millis() - rh_start > WINDOW_SIZE) rh_start = millis();// WINDOW_SIZE; //time to shift the Relay Window
  if (rh_output <= millis() - rh_start) {
    if (digitalRead(SOLENOID_VALVE_RELAY_PIN) == HIGH && millis() - last_change >= MIN_CHANGE_TIME) {
      digitalWrite(SOLENOID_VALVE_RELAY_PIN, !HIGH); //window off time (INVERTED LOGIC SOLENOID VS SSR)
      last_change = millis(); // Update the time of the last change
      // Serial.print("RH UP, with rh_output = "); Serial.println(rh_output);
    }
  }
  else {
    if (digitalRead(SOLENOID_VALVE_RELAY_PIN) == LOW && millis() - last_change >= MIN_CHANGE_TIME) {
      digitalWrite(SOLENOID_VALVE_RELAY_PIN, !LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)
      last_change = millis(); // Update the time of the last change
      // Serial.print("RH DOWN, with rh_output = "); Serial.println(rh_output);
    }
  }

  ih_PID.Compute();
  if (millis() - ih_start > WINDOW_SIZE) ih_start = millis(); //time to shift the Relay Window (ih_start = millis() also works)
  if (ih_output < millis() - ih_start) digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
  else digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)

  bh_PID.Compute();
  if (millis() - bh_start > WINDOW_SIZE) bh_start = millis(); //time to shift the Relay Window
  if (bh_output < millis() - bh_start) digitalWrite(BOX_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
  else digitalWrite(BOX_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)
}
