/* ======= Includes ======= */
#include <Arduino.h>
#include <Adafruit_GFX.h>     // base adafruit graphic lib required by the tft
#include <Adafruit_ILI9341.h> // library for the tft
#include <SPI.h>              // SPI 0 and 1 are used by the board itself, SPI 2 is used for the ethernet, must use eth 3 for tft screen
// May need an I2C library here
#include <ArduinoJson.h>
#include <stdint.h>
#include <string>
#include <Jikong_Handler.h>
#include <Pre_charge.h> // removeable?
// let GC & automation worry about MicroROS

/* ======= Compiler Switches ======= */
#define DEBUG_ENABLED 1              // compiler switch for debugging with a PC
#define NO_BMS 1                     // for testing without the BMS unit
#define DISABLE_DISCHARGE_ON_ERROR 0 // whether to disable rover power on BMS error

/* ======= Pin defs ======= */
// Use constexpr instead of #define, more useful for modern C++ at compile time
/*  Reserved / Do Not Use
  GPIO0        BOOT button (strapping pin)
  GPIO3        JTAG strap
  GPIO19/20    USB D-/D+ (native USB)
  GPIO21       Onboard RGB LED (RGB_DIN) — hardware trace
  GPIO22-25    Not bonded out on this chip package
  GPIO26-32    Internal Flash/PSRAM bus
  GPIO43/44    UART0 TX/RX — reserved for flashing/serial monitor
  GPIO45/46    Strapping pins (voltage select / boot mode)
*/
/*preset SPI pins for W5500 Ethernet are hardwired not changeable*/
constexpr uint8_t W5500_RST = 9;
constexpr uint8_t W5500_INT = 10;
constexpr uint8_t W5500_MOSI = 11;
constexpr uint8_t W5500_MISO = 12;
constexpr uint8_t W5500_SCLK = 13;
constexpr uint8_t W5500_CS = 14;

/*the SD card pins are also set in stone*/
constexpr uint8_t SD_CS = 4;
constexpr uint8_t SD_MISO = 5;
constexpr uint8_t SD_MOSI = 6;
constexpr uint8_t SD_CLK = 7;

/*UART 1 for BMS comms*/
constexpr uint8_t JIKONG_TX = 17;
constexpr uint8_t JIKONG_RX = 18;

/*IO Pins for comms with ATTiny85*/
// TODO:rename and assign pins when comms protocol is decided
//      if using I2C for comms, set SDA and SCL pins separately and add constant for I2C address
constexpr uint8_t ATTINY_1 = 34;
constexpr uint8_t ATTINY_2 = 35;

/*Pins for use with rotary encoder*/
constexpr uint8_t KY040_CLK = 8;
constexpr uint8_t KY040_DT = 15;
constexpr uint8_t KY040_SW = 16;

/*DEPRECATED Pins for comms with Precharge unit
constexpr uint8_t PRECHARGE_CH_A = 25; // GPIO19
constexpr uint8_t PRECHARGE_CH_B = 26; // GPIO20*/

/*SPI pins for TFT screen uses SPI3 via GPIO Matrix*/
constexpr uint8_t TFT_RST = 38;
constexpr uint8_t TFT_MOSI = 39;
constexpr uint8_t TFT_DC = 40;
constexpr uint8_t TFT_SCLK = 41;
constexpr uint8_t TFT_CS = 42;

// Free / Spare: GPIO pins 2, 8, 15, 33, 37, 47, 48

/* ======= Interrupt Flags ======= */
volatile bool screenUpdateFlag = false;
volatile bool getDataFlag = false;
volatile bool killFlag = false;
volatile bool encoderFlag = false;

/* ======= Globals ======= */

hw_timer_t *dataTimer = NULL;
hw_timer_t *screenTimer = NULL;

constexpr uint16_t BMS_COMMS_TIMEOUT_ms = 1000;
constexpr uint8_t numCells = 12;
constexpr uint8_t cellsPBattery = 6;
constexpr uint8_t numBatteries = 2;
JikongMessenger JKMessenger(&Serial2, BMS_COMMS_TIMEOUT_ms, numCells);

constexpr uint8_t MOSPassword[] = {0, 0, 0};
constexpr uint8_t passLength = sizeof(MOSPassword) / sizeof(MOSPassword[0]); // compute num elements in array
uint8_t passcodeAttempt[passLength] = {};
constexpr uint32_t ENCODER_DIGIT_DWELL_ms = 1000;
constexpr uint32_t ENCODER_ATTEMPT_TIMEOUT_ms = 5000;
volatile ulong lastEncoderChangeus = 0;
volatile bool encoderActive = false;

struct BMSDataStruct
{
  uint8_t batteryLife = 50;
  bool MOSStatus[2] = {0, 0}; // {charge, discharge} both 0 or 1
  int16_t packTemp = 25;      // is this per battery, there are 2?
  uint32_t cellVoltages[numCells] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t currentDraw = 0;
  uint16_t totalVoltage = 12;
  std::string error = ""; // append warning strings here as they are received
};

/* Each colour is enumerated by a 3-bit value where each bit marks whether
   the Red, Green, or Blue channel is lit (RGB)

   | Colour  | RGB Bits | Decimal |
   |---------|----------|---------|
   | Blue    | 001      | 1       |
   | Green   | 010      | 2       |
   | Cyan    | 011      | 3       |
   | Red     | 100      | 4       |
   | Magenta | 101      | 5       |
   | Yellow  | 110      | 6       |
   | White   | 111      | 7       |
*/
enum LEDStripColourEnum
{
  BLUE_MOTION = 0b001, // start from 1 so the bit mapping makes sense
  GREEN_MOTION_AUTO = 0b010,
  CYAN_MOTION_AUTO_DELAY = 0b011,
  RED_ERROR = 0b100,
  MAGENTA_STARTING_CONFLICT_ERROR = 0b101,
  YELLOW_LOCKED_INOPERABLE = 0b110,
  WHITE_SAFE_INTERACT = 0b111
};

/*DEPRECATED
PreCharge PreCharger(PRECHARGE_CH_A, PRECHARGE_CH_B);*/

enum EncoderDirectionsEnum
{
  CLOCKWISE,
  ANTICLOCKWISE
};

volatile bool encoderDirection = -1;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
constexpr uint8_t GET_DATA_RATE_HZ = 2; // Hz
constexpr uint8_t DISPLAY_UPDATE_HZ = 1;
const uint16_t screenWidth = tft.height(); // if rotation is odd, width and height swap
const uint16_t screenHeight = tft.width();
BMSDataStruct BMSData;
BMSDataStruct LastBMSData;
uint8_t encoderState = 0;
bool MOSSwitchSelected = false;

/* ======= Declare Functions ======= */

void getBMSData();
void getVerboseBMS();
void setLEDStripColour(LEDStripColourEnum colour);
void refreshDisplay();
void incrementPasscode(); // TODO
void checkPasscode();     // TODO
void killSwitch();
void resetESP();
void setMOSCharge(bool state);
void setMOSDischarge(bool state);
void encoderHandler();
void configureHWTimers(int dataRate = GET_DATA_RATE_HZ, int screenUpdateRate = DISPLAY_UPDATE_HZ);
void data_timer_ISR();
void screen_timer_ISR();
void encoder_CL_ISR();
void encoder_timeout_ISR(); // TODO, needs to redraw the main display, clear code progress, MOSSwitchSelected, encoderActive etc.

/* ======= The Program =======*/

void setup()
{
#if DEBUG_ENABLED
  Serial.begin(115200);
#endif
#if !NO_BMS
  Serial2.begin(115200, SERIAL_8N1, JIKONG_RX, JIKONG_TX);
#endif
  //  test ethernet connection (but dont error yet)
  //  delayed ethernet connection test as this board is upstream of it turning on
  //  test ROS2 connection
  //  test ATTiny connection(?)
  JKMessenger.begin(115200);

  setLEDStripColour(MAGENTA_STARTING_CONFLICT_ERROR);

  // initialise TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(1);

  // divide screen into 6 regions
  tft.drawFastHLine(0, screenHeight / 2, screenWidth, ILI9341_WHITE);
  tft.drawFastVLine(screenWidth / 3, 0, screenHeight, ILI9341_WHITE);
  tft.drawFastVLine(screenWidth * 2 / 3, 0, screenHeight, ILI9341_WHITE);

#if NO_BMS
  LastBMSData.batteryLife = 30;
  LastBMSData.MOSStatus[0] = 1;
  LastBMSData.MOSStatus[1] = 1;
  LastBMSData.packTemp = 30;
  for (size_t i = 0; i < 12; i++)
  {
    LastBMSData.cellVoltages[i] = 1;
  }
  LastBMSData.currentDraw = 1;
  LastBMSData.totalVoltage = 10;
  LastBMSData.error = "";
#endif

  pinMode(KY040_CLK, INPUT_PULLUP);
  pinMode(KY040_DT, INPUT_PULLUP);
  pinMode(KY040_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KY040_CLK), encoder_CL_ISR, FALLING);

  configureHWTimers();
}

void loop()
{
  static uint8_t lastState = HIGH;
  uint8_t state = digitalRead(KY040_CLK);
  if (state != lastState)
  {
    Serial.printf("KY040_CLK changed: %d -> %d\n", lastState, state);
    lastState = state;
  }
  if (killFlag)
  {
    killSwitch();
  }
  if (getDataFlag)
  {
#if (!NO_BMS)
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
        "Respectively; "
        "Current Draw is " +
        String(BMSData.currentDraw) + "; " +
        "Total Voltage is: " + String(BMSData.totalVoltage) +
        "; Error is: " + BMSData.error.c_str() + "\n");
#endif
    screenUpdateFlag = false;
  }
}

void getBMSData()
{
  JKMessenger.request_data();

  const auto *warningFlags = JKMessenger.get_warning_flags();
  BMSData.error.clear();

  // map values to labels
  struct WarningEntry
  {
    const char *label;
    bool JikongMessenger::Warning_Flags::*member;
  };

  const WarningEntry warningEntries[] = {
      {"Low capacity", &JikongMessenger::Warning_Flags::low_capacity},
      {"MOS tube overtemp", &JikongMessenger::Warning_Flags::MOS_tube_OT},
      {"Charging overvoltage", &JikongMessenger::Warning_Flags::charging_OV},
      {"Discharge undervoltage", &JikongMessenger::Warning_Flags::discharge_UV},
      {"Battery overtemp", &JikongMessenger::Warning_Flags::battery_OT},
      {"Charging overcurrent", &JikongMessenger::Warning_Flags::charging_OC},
      {"Discharge overcurrent", &JikongMessenger::Warning_Flags::discharge_OC},
      {"Cell pressure differential", &JikongMessenger::Warning_Flags::Cell_pressure_differential},
      {"Battery box overtemp", &JikongMessenger::Warning_Flags::BB_OT},
      {"Battery low temp", &JikongMessenger::Warning_Flags::battery_low_temp},
      {"Cell overvoltage", &JikongMessenger::Warning_Flags::monomer_OV},
      {"Cell undervoltage", &JikongMessenger::Warning_Flags::monomer_UV},
      {"Protection 309A", &JikongMessenger::Warning_Flags::protection_309A},
  };

  for (const auto &entry : warningEntries) // for entry in warningEntries
  {
    if (warningFlags && warningFlags->*entry.member)
    {
      if (!BMSData.error.empty())
      {
        BMSData.error += "; ";
      }
      BMSData.error += entry.label;
    }
  }

  // error checking
  if (BMSData.error != "")
  {
    setLEDStripColour(RED_ERROR);
#if DISABLE_DISCHARGE_ON_ERROR
    JKMessenger.setMOS_state(false, false);
#endif
    tft.setCursor(screenHeight / 2, 0);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.setTextSize(3);
    tft.setTextWrap(1);
    tft.print("BMS Errors Detected: " + String(BMSData.error.c_str()) + "\n"); // list errors
  }

  BMSData.batteryLife = JKMessenger.get_remaining_capacity_pct();
  const JikongMessenger::Cell_Voltages *cellVoltages = JKMessenger.get_cell_voltage_mV();
  for (size_t i = 0; i < numCells; i++)
  {
    BMSData.cellVoltages[i] = cellVoltages[i].cellVoltage;
  }
  BMSData.currentDraw = JKMessenger.get_current_dA();
  BMSData.MOSStatus[0] = JKMessenger.get_status_flags()->charging_MOS_status;
  BMSData.MOSStatus[1] = JKMessenger.get_status_flags()->discharge_MOS_status;
  BMSData.packTemp = JKMessenger.get_battery_temp_dC();
  BMSData.totalVoltage = JKMessenger.get_total_voltage_mV();
}

void getVerboseBMS()
{
  // TODO: retrieve all getter values to some format for export (JSON string?)
  JKMessenger.request_data();

  const auto *warningFlags = JKMessenger.get_warning_flags();
  BMSData.error.clear();

  // map values to labels
  struct WarningEntry
  {
    const char *label;
    bool JikongMessenger::Warning_Flags::*member;
  };

  const WarningEntry warningEntries[] = {
      {"Low capacity", &JikongMessenger::Warning_Flags::low_capacity},
      {"MOS tube overtemp", &JikongMessenger::Warning_Flags::MOS_tube_OT},
      {"Charging overvoltage", &JikongMessenger::Warning_Flags::charging_OV},
      {"Discharge undervoltage", &JikongMessenger::Warning_Flags::discharge_UV},
      {"Battery overtemp", &JikongMessenger::Warning_Flags::battery_OT},
      {"Charging overcurrent", &JikongMessenger::Warning_Flags::charging_OC},
      {"Discharge overcurrent", &JikongMessenger::Warning_Flags::discharge_OC},
      {"Cell pressure differential", &JikongMessenger::Warning_Flags::Cell_pressure_differential},
      {"Battery box overtemp", &JikongMessenger::Warning_Flags::BB_OT},
      {"Battery low temp", &JikongMessenger::Warning_Flags::battery_low_temp},
      {"Cell overvoltage", &JikongMessenger::Warning_Flags::monomer_OV},
      {"Cell undervoltage", &JikongMessenger::Warning_Flags::monomer_UV},
      {"Protection 309A", &JikongMessenger::Warning_Flags::protection_309A},
  };

  for (const auto &entry : warningEntries) // for entry in warningEntries
  {
    if (warningFlags && warningFlags->*entry.member)
    {
      if (!BMSData.error.empty())
      {
        BMSData.error += "; ";
      }
      BMSData.error += entry.label;
    }
  }

  // error checking
  if (BMSData.error != "")
  {
    noInterrupts();
    setLEDStripColour(RED_ERROR);
#if DISABLE_DISCHARGE_ON_ERROR
    JKMessenger.setMOS_state(false, false);
#endif
    tft.setCursor(screenHeight / 2, 0);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.setTextSize(3);
    tft.setTextWrap(1);
    tft.print("BMS Errors Detected: " + String(BMSData.error.c_str()) + "\n"); // list errors
    interrupts();
  }

  BMSData.batteryLife = JKMessenger.get_remaining_capacity_pct();
  const JikongMessenger::Cell_Voltages *cellVoltages = JKMessenger.get_cell_voltage_mV();
  for (size_t i = 0; i < numCells; i++)
  {
    BMSData.cellVoltages[i] = cellVoltages[i].cellVoltage;
  }
  BMSData.currentDraw = JKMessenger.get_current_dA();
  BMSData.MOSStatus[0] = JKMessenger.get_status_flags()->charging_MOS_status;
  BMSData.MOSStatus[1] = JKMessenger.get_status_flags()->discharge_MOS_status;
  BMSData.packTemp = JKMessenger.get_battery_temp_dC();
  BMSData.totalVoltage = JKMessenger.get_total_voltage_mV();
}

void refreshDisplay() // TODO: add colours to text where relevant
{
  if (!MOSSwitchSelected)
  {

    // casts are defensive for the division, shouldnt come into play
    const uint16_t columnWidth = static_cast<uint16_t>(screenWidth / 3);
    const uint16_t rowHeight = static_cast<uint16_t>(screenHeight / 2);
    constexpr uint16_t margin = 2;

    // '[&]' allows to access local vars
    auto printSection = [&](const String &label, const String &value, uint16_t left, uint16_t top, uint16_t valueSize = 4, uint16_t valueColor = ILI9341_WHITE)
    {
      tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
      tft.setCursor(left + margin, top);
      tft.println(label);
      tft.setTextSize(valueSize);
      tft.setTextColor(valueColor, ILI9341_BLACK);
      tft.setCursor(left + margin, top + 10);
      tft.println(value);
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    };

    tft.setTextSize(1);
    if (LastBMSData.totalVoltage != BMSData.totalVoltage)
    {
      printSection("Total Voltage", String(BMSData.totalVoltage) + "mV", 0, 2);
    }
    if (LastBMSData.packTemp != BMSData.packTemp)
    {
      printSection("Pack Temp", String(BMSData.packTemp) + "C", columnWidth + 1, 2);
    }
    if (LastBMSData.currentDraw != BMSData.currentDraw)
    {
      printSection("Current Draw", String(BMSData.currentDraw) + "A", columnWidth * 2 + 2, 2);
    }
    if (LastBMSData.MOSStatus[0] != BMSData.MOSStatus[0] ||
        LastBMSData.MOSStatus[1] != BMSData.MOSStatus[1])
    {
      printSection("Charge MOS", BMSData.MOSStatus[0] ? "Enabled" : "Disabled", 0, rowHeight + 3, 2,
                   BMSData.MOSStatus[0] ? ILI9341_GREEN : ILI9341_RED);
      printSection("Discharge MOS", BMSData.MOSStatus[1] ? "Enabled" : "Disabled", 0, rowHeight + 26 + 3, 2,
                   BMSData.MOSStatus[1] ? ILI9341_GREEN : ILI9341_RED);
      tft.setTextColor(ILI9341_WHITE);
    }
    bool cellVoltagesChanged = false;
    for (size_t i = 0; i < numCells; i++)
    {
      if (LastBMSData.cellVoltages[i] != BMSData.cellVoltages[i])
      {
        cellVoltagesChanged = true;
        break;
      }
    }
    if (cellVoltagesChanged)
    {
      // these static casts stop the compiler yelling at me
      const uint16_t cellX[cellsPBattery / 2] = {
          static_cast<uint16_t>(columnWidth + 8),
          static_cast<uint16_t>(columnWidth + 38),
          static_cast<uint16_t>(columnWidth + 68)};
      const uint16_t cellY[numBatteries][cellsPBattery / 3] = {
          {static_cast<uint16_t>(rowHeight + 20), static_cast<uint16_t>(rowHeight + 32)},
          {static_cast<uint16_t>(rowHeight + 76), static_cast<uint16_t>(rowHeight + 88)}};

      tft.setCursor(columnWidth + 3, rowHeight + 3);
      tft.println("Battery 1 mV/cell");
      tft.setCursor(columnWidth + 3, rowHeight + 58 + 3);
      tft.println("Battery 2 mV/cell");

      for (size_t battery = 0; battery < numBatteries; battery++)
      {
        for (size_t row = 0; row < 2; row++)
        {
          for (size_t column = 0; column < 3; column++)
          {
            size_t index = battery * cellsPBattery + row * 3 + column;
            tft.setCursor(cellX[column], cellY[battery][row]);
            tft.print(BMSData.cellVoltages[index]);
          }
        }
      }
      tft.setTextColor(ILI9341_WHITE);
    }
    if (LastBMSData.batteryLife != BMSData.batteryLife)
    {
      printSection("Battery Life", String(BMSData.batteryLife) + "%", columnWidth * 2 + 2, rowHeight + 3);
    }
  }
}

void setLEDStripColour(LEDStripColourEnum colour)
{
  // TODO: (tell ATTiny to?) set LEDstrip colour to colour value
}

// TODO: enable/disable with rotary encoder combo

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

void encoderHandler()
{
  if (encoderDirection) // encoder has 20 positions
  {
    encoderState = (encoderState - 1) % 19; // decrement, wrapping at 0
  }
  else
  {
    encoderState = (encoderState + 1) % 19;
  }
#if DEBUG_ENABLED
  Serial.println("Encoder Direction: " + String(encoderDirection ? "Anticlockwise" : " Clockwise"));
  Serial.println("Encoder State: " + String(encoderState));
#endif
  /* TODO: handle encoder logic

  desired flow:
    - select mos mode to change by rotating dial(encoder) and stopping on desired setting for x duration
      - set an enum flag for main display loop to read and highlight the currently selected MOS setting somehow
    - clear screen (once per MOSSwitchSelected true), display current encoder position on screen numerically in large size
    - password input should function like like a physical lock with a dial
      - encoderState saves to current attempt buffer on direction change (or maybe stop for a duration or use encoder button?)
      - last digit must be stopped on for x duration for its entry to be confirmed

    - each encoder state change starts/resets a counter for a timeout if more than z seconds passes since last input
    - all encoder inputs should be bidirectional
    - if an additional input button is required the encoder can be pressed in for a signal on the SW pin
      - encoder button is NOT debounced

    check which direction turned, increment combo, when entered digits reaches pass length check password, and if correct, toggle selected MOS state */
  encoderFlag = false;
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

  // TODO: encoder timout timer - reset clock in encoder pin interrupt to keep from triggering
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
  encoderDirection = digitalRead(KY040_DT) ? CLOCKWISE : ANTICLOCKWISE; // if clockwise, CL pin will go low first, and visa versa

  uint32_t now = micros();
  // if (now - lastEncoderChangeus < 1500)
  // {
  //   return;
  // }
  lastEncoderChangeus = now;
  encoderActive = true;
  encoderFlag = true;
}