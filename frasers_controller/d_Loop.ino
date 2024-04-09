
void loop() {
  Runner.execute(); //handles sensor readings and sending data over Serial

  wdt_reset(); //keeps watchdog from performing a software reset
  if (Serial.available() > 0) handleUserInput();

  if ( millis() > WATCHDOG_TIMEOUT_PERIOD ) delay(5000); //software reset of Arduino using watchdog timer

  rh_PID.Compute();
  if (millis() - rh_start > WINDOW_SIZE) rh_start = millis();// WINDOW_SIZE; //time to shift the Relay Window
  if (rh_output > millis() - rh_start && rh_output > MIN_CHANGE_TIME) digitalWrite(SOLENOID_VALVE_RELAY_PIN, HIGH); //NOTE THE >/<
  else digitalWrite(SOLENOID_VALVE_RELAY_PIN, LOW);

  ih_PID.Compute();
  if (millis() - ih_start > WINDOW_SIZE) ih_start = millis(); //time to shift the Relay Window (ih_start = millis() also works)
  if (ih_output < millis() - ih_start && water_temperature >= -0.5) digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR). 
  else digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)

  bh_PID.Compute();
  if (millis() - bh_start > WINDOW_SIZE) bh_start = millis(); //time to shift the Relay Window
  if (bh_output < millis() - bh_start && sht.readSample()) digitalWrite(BOX_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
  else digitalWrite(BOX_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)
}
