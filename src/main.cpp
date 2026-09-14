/* ======= Includes ======= */
#include <Arduino.h>
#include <Adafruit_GFX.h>     // base adafruit graphic lib required by the tft
#include <Adafruit_ILI9341.h> // library for the tft
#include <SPI.h>              // SPI 0 and 1 are used by the board itself, SPI 2 is used for the ethernet, must use eth 3 for tft screen
#include <ArduinoJson.h>
#include <stdint.h>
#include <string>
#include <Jikong_Handler.h>
#include <Pre_charge.h>
// let GC & automation worry about MicroROS

/* ======= Compiler Switches ======= */
#define DEBUG_ENABLED 1              // compiler switch for debugging with a PC
#define NO_BMS 1                     // for testing without the BMS unit
#define DISABLE_DISCHARGE_ON_ERROR 0 // whether to disable rover power on BMS error

/* ======= Pin defs ======= */
// Use constexpr instead of #define, more useful for modern C++ at compile time

/*preset SPI pins for W5500 Ethernet are hardwired not changeable*/
constexpr uint8_t W5500_RST = 14;  // GPIO9
constexpr uint8_t W5500_INT = 15;  // GPIO10
constexpr uint8_t W5500_MOSI = 16; // GPIO11
constexpr uint8_t W5500_MISO = 17; // GPIO12
constexpr uint8_t W5500_SCLK = 18; // GPIO13
constexpr uint8_t W5500_CS = 19;   // GPIO14

/*the SD card pins are also set in stone if it is in use*/
constexpr uint8_t SD_CS = 9;    // GPIO4
constexpr uint8_t SD_MISO = 10; // GPIO5
constexpr uint8_t SD_MOSI = 11; // GPIO6
constexpr uint8_t SD_CLK = 12;  // GPIO7

/*UART 1 for BMS comms*/
constexpr uint8_t JIKONG_TX = 23; // GPIO17
constexpr uint8_t JIKONG_RX = 24; // GPIO18

/*SPI pins for TFT screen uses SPI3 which must be routed via GPIO matrix, meaning they can be assigned to pretty much any  unused pins*/ // TODO: learn how to and implement this matrix stuff
// values are placeholders
constexpr uint8_t TFT_RST = 38;  // GPIO33
constexpr uint8_t TFT_MOSI = 39; // GPIO34
constexpr uint8_t TFT_DC = 40;   // GPIO35
constexpr uint8_t TFT_SCLK = 41; // GPIO36
constexpr uint8_t TFT_CS = 42;   // GPIO37

/* ======= Interrupt Flags ======= */
volatile bool screenUpdateFlag = true;
volatile bool getDataFlag = false;

/* ======= Globals ======= */

constexpr uint16_t BMS_COMMS_TIMEOUT_ms = 1000;
constexpr uint8_t numCells = 12;
constexpr uint8_t cellsPBattery = 6;
constexpr uint8_t numBatteries = 2;
JikongMessenger JKMessenger(&Serial2, BMS_COMMS_TIMEOUT_ms, numCells);

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
// PreCharge PreCharger(,);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
constexpr uint8_t getDataRate = 2; // Hz
constexpr uint8_t displayUpdateHz = 1;
const uint16_t screenWidth = tft.height(); // if rotation is odd, width and height swap
const uint16_t screenHeight = tft.width();
BMSDataStruct BMSData;
BMSDataStruct LastBMSData;

/* ======= Declare Functions ======= */

void getBMSData();
void getVerboseBMS();
void setLEDStripColour(LEDStripColourEnum colour);
void refreshDisplay();
void incrementPasscode();
void checkPasscode();
void killSwitch();
void resetESP();
void setMOSCharge(bool state);
void setMOSDischarge(bool state);

/* ======= The Program =======*/

void setup()
{
#if DEBUG_ENABLED
  Serial.begin(115200);
#endif
  // Serial2.begin(115200, SERIAL_8N1, JIKONG_RX, JIKONG_TX);
  //  test precharge connection
  //  test ethernet connection
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
}

void loop()
{
  if (getDataFlag)
  {
    // getBMSData();
  }
  if (screenUpdateFlag)
  { // update flag driven by a hardware clock
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
  }
}

void getBMSData()
{
  // TODO: link to BMS comms and assign values to struct
  JKMessenger.request_data();
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
  else
  {
  }
}

void refreshDisplay() // TODO: add colours to text where relevant
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

void setLEDStripColour(LEDStripColourEnum colour)
{
  // (tell ATTiny to?) set LEDstrip colour to colour value
}

// TODO: enable/disable with rotary encoder combo
// TODO: hardware timer interrupts for pulling data (?Hz) and refreshing the screen (1Hz)