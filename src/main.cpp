/* ======= Includes ======= */
#include <Arduino.h>
#include <SPI.h> // SPI 0 and 1 are used by the board itself, SPI 2 is used for the ethernet, must use eth 3 for tft screen
#include <Wire.h>
#include <stdint.h>
#include <string>
#include <Jikong_Handler.h>
#include <Pre_charge.h> // removeable?
#include <BMSData.h>
#include <Display.h>
#include <settings.h> // globals and definitions in settings.h
// let GC & automation worry about MicroROS

/* ======= Declare Functions ======= */

bool setLEDStripColour(LEDStripColourEnum colour);
void incrementPasscode();
void checkPasscode();
void killSwitch();
void resetESP();
void setMOSCharge(bool state);
void setMOSDischarge(bool state);
void encoderHandler();
void resetToIdle();
void setScreenUpdateRate(uint8_t hz);
void configureHWTimers(int dataRate = GET_DATA_RATE_HZ, int screenUpdateRate = DISPLAY_UPDATE_HZ);
void data_timer_ISR();
void screen_timer_ISR();
void encoder_CL_ISR();

/* ======= The Program =======*/

void setup()
{
#if DEBUG_ENABLED
  Serial.begin(115200);
#endif
#if BMS_ENABLED
  Serial2.begin(115200, SERIAL_8N1, JIKONG_RX, JIKONG_TX);
#endif
#if ATTINY_ENABLED
  if not(Wire.begin(ATTINY_SDA, ATTINY_SCL, 100000))
  {
#if DEBUG_ENABLED
    Serial.println("ATTiny connection failed");
#endif
  }
#endif
  //  delayed ethernet connection test (as function elsewhere, called a duration after discharge enabled) as this board is upstream of it turning on
  //  test ROS2 connection
  JKMessenger.begin(115200);

#if ATTINY_ENABLED
  setLEDStripColour(MAGENTA_STARTING_CONFLICT_ERROR);
#endif

  // initialise TFT
  beginDisplay();
  segmentDisplay();

#if not BMS_ENABLED
  getDummyBMS();
#endif

  pinMode(KY040_CLK, INPUT);
  pinMode(KY040_DT, INPUT);
  pinMode(KY040_SW, INPUT);
  attachInterrupt(digitalPinToInterrupt(KY040_CLK), encoder_CL_ISR, RISING);

  configureHWTimers();
}

void loop()
{

  if (killFlag)
  {
    killSwitch();
  }
  if (encoderActive && (micros() - lastEncoderChangeus) >= (ENCODER_ATTEMPT_TIMEOUT_ms * 1000UL))
  {
    resetToIdle();
#if DEBUG_ENABLED
    Serial.println("Encoder timed out — resetting to main display");
#endif
  }

  static bool lastEncoderActiveForTimer = false;
  if (encoderActive != lastEncoderActiveForTimer) // make screen more responsive when user interacting
  {
    lastEncoderActiveForTimer = encoderActive;
    setScreenUpdateRate(encoderActive ? ENCODER_ACTIVE_DISPLAY_UPDATE_HZ : DISPLAY_UPDATE_HZ);
#if DEBUG_ENABLED
    Serial.println(encoderActive ? "Screen refresh: 30Hz (encoder active)" : "Screen refresh: 1Hz (idle)");
#endif
  }

  /* Dwell-based confirmation: if the encoder has sat still for ENCODER_DIGIT_DWELL_ms while a selection/entry is in progress, treat the current value as confirmed.
     Alternatives considered: confirm on direction reversal, or confirm via a press on the encoder's SW pin  */
  if (encoderActive && encoderMovedSinceStageStart &&
      (micros() - lastEncoderChangeus) >= (ENCODER_DIGIT_DWELL_ms * 1000UL))
  {
    if (encoderMode == MODE_SELECT_MOS)
    {
      MOSSwitchSelected = true;
      encoderMode = MODE_PASSCODE_ENTRY;
      passcodeIndex = 0;
      encoderState = 0;
      encoderMovedSinceStageStart = false;
      drawPasscodeDigit();
      drawPasscodeDots();
      lastEncoderChangeus = micros(); // restart the dwell window for digit 1
    }
    else if (encoderMode == MODE_PASSCODE_ENTRY)
    {
      incrementPasscode();
    }
  }

  if (getDataFlag)
  {
#if (BMS_ENABLED)
    getBMSData();
#endif
    getDataFlag = false;
  }
  if (encoderFlag)
  {
#if DEBUG_ENABLED
    Serial.println("Encoder interrupt triggered");
#endif
    encoderHandler();
  }
  if (screenUpdateFlag)
  {
    refreshDisplay();
    LastBMSData = BMSData;
#if DEBUG_ENABLED
    Serial.print( // print data to serial
        "Battery Life is: " + String(BMSData.batteryLife) + "; " +
        "MOS Status is: " + String(BMSData.MOSStatus[0]) + ", " + String(BMSData.MOSStatus[1]) + "; " +
        "Pack Temperature is: " + String(BMSData.packTemp) + "; " +
        "Cell Voltages are: ");
    for (int i = 0; i < 12; i++)
    {
      Serial.print(String(BMSData.cellVoltages[i]));
      if (i < 11)
      {
        Serial.print(", ");
      }
    }
    Serial.print(
        " Respectively; "
        "Current Draw is " +
        String(BMSData.currentDraw) + "; " +
        "Total Voltage is: " + String(BMSData.totalVoltage) +
        "; Error is: " + BMSData.error.c_str() + "\n");
#endif
    screenUpdateFlag = false;
  }
}

bool setLEDStripColour(LEDStripColourEnum colour)
{
  uint8_t errorCount = 0;
  while (errorCount <= LED_UPDATE_RETRIES)
  {
    uint8_t error = 0;
    Wire.beginTransmission(ATTINY_ADDR);
    Wire.write(colour); // since the ATTiny will only be controlling the LED strip, I only need to transmit a colour which is also defined at the other end
    error = Wire.endTransmission();

    if (error = 0)
    {
      return true;
    }
#if DEBUG_ENABLED
    Serial.print("I2C error: ");
    Serial.println(error);
    Serial.println("Retrying...");
#endif
    errorCount++;
  }
#if DEBUG_ENABLED
  Serial.printf("LED Update Failed after %d retries...\n", LED_UPDATE_RETRIES);
#endif
#if ROS_ENABLED
// TODO: tell ROS that the colour update failed
#endif
  return false;
}

void killSwitch()
{
  JKMessenger.setMOS_state(false, false);
}

void resetESP()
{
  ESP.restart();
}

void setMOSCharge(bool state)
{
  JKMessenger.request_data();
  BMSData.MOSStatus[1] = JKMessenger.get_status_flags()->discharge_MOS_status;
  JKMessenger.setMOS_state(state, BMSData.MOSStatus[1]);
}

void setMOSDischarge(bool state)
{
  JKMessenger.request_data();
  BMSData.MOSStatus[0] = JKMessenger.get_status_flags()->charging_MOS_status;
  JKMessenger.setMOS_state(BMSData.MOSStatus[0], state);
}

void encoderHandler() // TODO: fix this mess
{
  // Wrap modulus depends on what the user is currently doing with the dial
  uint8_t modulus;
  switch (encoderMode)
  {
  case MODE_SELECT_MOS:
    modulus = 2;
    break;
  case MODE_PASSCODE_ENTRY:
    modulus = 20;
    break;
  case MODE_IDLE:
  default:
    // First movement from idle starts a MOS-select interaction
    encoderMode = MODE_SELECT_MOS;
    modulus = 2;
    encoderState = 0;
    break;
  }

  if (encoderDirection) // anticlockwise
  {
    encoderState = (encoderState + modulus - 1) % modulus;
  }
  else // clockwise
  {
    encoderState = (encoderState + 1) % modulus;
  }

#if DEBUG_ENABLED
  Serial.println("Encoder Direction: " + String(encoderDirection ? "Anticlockwise" : "Clockwise"));
  Serial.println("Encoder State: " + String(encoderState));
#endif

  switch (encoderMode)
  {
  case MODE_SELECT_MOS:
    selectedMOS = static_cast<SelectedMOSEnum>(encoderState);
    screenUpdateFlag = true;
    break;
  case MODE_PASSCODE_ENTRY:
    drawPasscodeDigit();
    break;
  default:
    break;
  }

  encoderFlag = false;
}

void incrementPasscode()
{
  // Store as 1-20 to match MOSPassword's range, not the raw 0-19 value
  passcodeAttempt[passcodeIndex] = encoderState + 1;
  passcodeIndex++;
  encoderMovedSinceStageStart = false; // next digit needs its own real turn

  if (passcodeIndex >= passLength)
  {
    checkPasscode();
  }
  else
  {
    drawPasscodeDigit();
    drawPasscodeDots();
  }
}

void checkPasscode()
{
  bool correct = true;
  for (uint8_t i = 0; i < passLength; i++)
  {
    if (passcodeAttempt[i] != MOSPassword[i])
    {
      correct = false;
      break;
    }
  }

  if (correct) // if correct toggle state
  {
    if (selectedMOS == SEL_CHARGE)
    {
      setMOSCharge(!BMSData.MOSStatus[0]);
    }
    else
    {
      setMOSDischarge(!BMSData.MOSStatus[1]);
    }
  }
  resetToIdle();
}

void resetToIdle()
{
  encoderActive = false;
  encoderMovedSinceStageStart = false;
  encoderMode = MODE_IDLE;
  MOSSwitchSelected = false;
  codeScreenTriggered = false;
  passcodeIndex = 0;
  for (size_t i = 0; i < passLength; i++)
  {
    passcodeAttempt[i] = 0;
  }

  segmentDisplay();
  LastBMSData = BMSDataStruct(); // force refreshDisplay() to repaint every section
#if not BMS_ENABLED
  getDummyBMS();
#endif
  screenUpdateFlag = true;
}

void setScreenUpdateRate(uint8_t hz)
{
  const uint32_t periodUs = static_cast<uint32_t>(1000000ULL / hz);
  timerAlarmWrite(screenTimer, periodUs, true);
}

void configureHWTimers(int dataRate, int screenUpdateRate)
{

  /* The default APB clock is 80 MHz. Dividing by the prescaler gives a 1 MHz
   timer clock, so each tick is 1 µs. The period between triggers is
   1 / dataRate seconds, which is converted to timer ticks by multiplying by
   1,000,000. Use floating-point division here so a 2 Hz timer does not get
   truncated to zero ticks. */

  const uint32_t dataPeriodUs = static_cast<uint32_t>(1000000ULL / dataRate);
  const uint32_t screenPeriodUs = static_cast<uint32_t>(1000000ULL / screenUpdateRate);

  /* Data polling timer */
  // create timer
  dataTimer = timerBegin(0, 80, true); // timer 0, prescaler of 80, counting up

  // attach interrupt
  timerAttachInterrupt(dataTimer, &data_timer_ISR, true);

  // set alarm(interrupt) to trigger on an interval
  timerAlarmWrite(dataTimer, dataPeriodUs, true);
  timerAlarmEnable(dataTimer);

  /* Screen update timer */
  screenTimer = timerBegin(1, 80, true); // timer 1
  timerAttachInterrupt(screenTimer, &screen_timer_ISR, true);
  timerAlarmWrite(screenTimer, screenPeriodUs, true);
  timerAlarmEnable(screenTimer);
}

void IRAM_ATTR data_timer_ISR()
{
  getDataFlag = true;
}

void IRAM_ATTR screen_timer_ISR()
{
  screenUpdateFlag = true;
}

void IRAM_ATTR encoder_CL_ISR()
{
  encoderDirection = (digitalRead(KY040_DT) == HIGH) ? ANTICLOCKWISE : CLOCKWISE;
  lastEncoderChangeus = micros();

  encoderActive = true;
  encoderMovedSinceStageStart = true;
  encoderFlag = true;
}