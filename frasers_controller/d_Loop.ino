
void loop() {
  Runner.execute(); //handles sensor readings and sending data over Serial

  wdt_reset(); //keeps watchdog from performing a software reset
  if (Serial.available() > 0) handleUserInput();

  if ( millis() > WATCHDOG_TIMEOUT_PERIOD ) delay(5000); //software reset of Arduino using watchdog timer

 //*NOTE: trying inverting "digitalWrite(..., HIGH)" and "digitalWrite(..., LOW)" if you are seeing unexpected behavior.
  rh_PID.Compute();
  if (rh_output > WINDOW_SIZE-MIN_WINDOW) rh_output = WINDOW_SIZE; //quickest change 0.5s
  else if (rh_output <= MIN_WINDOW) rh_output = 0; //quickest change 0.5s
  if (millis() - rh_start > WINDOW_SIZE) rh_start += WINDOW_SIZE; //time to shift the Relay Window
  if (rh_output < millis() - rh_start) digitalWrite(SOLENOID_VALVE_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
  else digitalWrite(SOLENOID_VALVE_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)

  ih_PID.Compute();
  if (ih_output > WINDOW_SIZE-MIN_WINDOW) ih_output = WINDOW_SIZE; //can change no faster than MIN_WINDOW
  else if (ih_output <= MIN_WINDOW) ih_output = 0; //can change no faster than WINDOW_SIZE
  if (millis() - ih_start > WINDOW_SIZE) ih_start += WINDOW_SIZE; //time to shift the Relay Window (ih_start = millis() also works)
  if (ih_output < millis() - ih_start) digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
  else digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)

  bh_PID.Compute();
  if (bh_output > WINDOW_SIZE-MIN_WINDOW) bh_output = WINDOW_SIZE; //quickest change 0.5s
  else if (bh_output <= MIN_WINDOW) bh_output = 0; //quickest change 0.5s
  if (millis() - bh_start > WINDOW_SIZE) bh_start += WINDOW_SIZE; //time to shift the Relay Window
  if (bh_output < millis() - bh_start) digitalWrite(BOX_HEATER_RELAY_PIN, HIGH); //window on time (INVERTED LOGIC SOLENOID VS SSR)
  else digitalWrite(BOX_HEATER_RELAY_PIN, LOW); //window off time (INVERTED LOGIC SOLENOID VS SSR)
}
