
void loop() {
  Runner.execute(); //handles sensor readings and sending data over Serial


  if (Serial.available() > 0) handleUserInput();

 //*NOTE: trying inverting "digitalWrite(..., HIGH)" and "digitalWrite(..., LOW)" if you are seeing unexpected behavior.
  rh_PID.Compute();
  if (rh_output > 4.5) ih_output = 5; //quickest change 0.5s
  if (rh_output < 0.5) ih_output = 0; //quickest change 0.5s
  if (millis() - rh_start > WINDOW_SIZE) rh_start += WINDOW_SIZE; //time to shift the Relay Window
  if (rh_output < millis() - rh_start) digitalWrite(SOLENOID_VALVE_RELAY_PIN, HIGH); //window on time
  else digitalWrite(SOLENOID_VALVE_RELAY_PIN, LOW); //window off time

  if(digitalRead(WATER_LEVEL_PIN)) { //if no water in the cup, turn off immersion heater and flash LED as an alert
    flashLED();
    digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); 
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
    ih_PID.Compute();
    if (ih_output > 4.5) ih_output = 5; //quickest change 0.5s
    if (ih_output < 0.5) ih_output = 0; //quickest change 0.5s
    if (millis() - ih_start > WINDOW_SIZE) ih_start += WINDOW_SIZE; //time to shift the Relay Window (ih_start = millis() also works)
    if (ih_output < millis() - ih_start) digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); //window on time
    else digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH); //window off time
  }
  // ih_PID.Compute();
  // if (millis() - ih_start > WINDOW_SIZE) ih_start += WINDOW_SIZE; //time to shift the Relay Window (ih_start = millis() also works)
  // if (ih_output < millis() - ih_start) digitalWrite(IMMERSION_HEATER_RELAY_PIN, HIGH); //window on time
  // else digitalWrite(IMMERSION_HEATER_RELAY_PIN, LOW); //window off time

  bh_PID.Compute();
  if (bh_output > 4.5) ih_output = 5; //quickest change 0.5s
  if (bh_output < 0.5) ih_output = 0; //quickest change 0.5s
  // Serial.print("bh output: "); Serial.println(bh_output);
  if (millis() - bh_start > WINDOW_SIZE) bh_start += WINDOW_SIZE; //time to shift the Relay Window
  if (bh_output < millis() - bh_start) digitalWrite(BOX_HEATER_RELAY_PIN, LOW); //window on time
  else digitalWrite(BOX_HEATER_RELAY_PIN, HIGH); //window off time

}




/*
**Key to commands for serial input:
First letters are either i, b, or r, which stand for "immersion heater", "box heater", and "relative humidity", respectively.
Second letters are either p, i, d, or s, which stand for Kp, Ki, Kd, or Setpoint, respectively.
Last part...
"?" returns what that value currently is.
"+" adds 0.1 to current value and returns new value.
"++" adds 1 to current value and returns new value.
"-" subtracts 0.1 from current value and returns new value.
"--" subtracts 1 from current value and return new value.
"=xx" sets the value to whatever number you like, i.e. "=-12.4" sets it to -12.4.
*NOTE: you can't enter "=0". "=0.00001" is effectively the same. if you want 0, you can do "=0.1" and then "-". 

**List of commands for serial input:
ip?
ip+
ip++
ip-
ip--
ip=xx
------
ii?
ii+
ii++
ii-
ii--
ii=xx
------
id?
id+
id++
id-
id--
id=xx
------
is?
is=xx
++++++++
bp?
bp+
bp++
bp-
bp--
bp=xx
------
bi?
bi+
bi++
bi-
bi--
bi=xx
------
bd?
bd+
bd++
bd-
bd--
bd=xx
------
bs?
bs=xx
++++++++
rp?
rp+
rp++
rp-
rp--
rp=xx
------
ri?
ri+
ri++
ri-
ri--
ri=xx
------
rd?
rd+
rd++
rd-
rd--
rd=xx
------
rs?
rs=xx

*/