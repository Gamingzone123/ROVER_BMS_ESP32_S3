/* ======= Includes ======= */
#include <Arduino.h>
#include <Adafruit_GFX.h>     // base adafruit graphic lib required by the tft
#include <Adafruit_ILI9341.h> // library for the tft
#include <SPI.h>              // SPI 0 and 1 are used by the board itself, SPI 2 is used for the ethernet, must use eth 3 for tft screen
#include <ArduinoJson.h>
#include <stdint.h>
#include <Jikong_Handler.h>
#include <Pre_charge.h>
// let GC & automation worry about MicroROS

/* ======= Compiler Switches ======= */
#define DEBUG_ENABLED 0              // compiler switch for debugging with a PC
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

uint16_t BMS_COMMS_TIMEOUT_ms = 1000;
constexpr uint8_t numCells = 12;
constexpr uint8_t cellsPBattery = 6;
constexpr uint8_t numBatteries = 2;
JikongMessenger JKMessenger(&Serial2, BMS_COMMS_TIMEOUT_ms, numCells);

struct BMSDataStruct
{
  uint8_t batteryLife = 50;
  bool MOSStatus[2] = {0, 0};                                             // {charge, discharge} both 0 or 1
  int16_t packTemp = {25};                                                // is this per battery, there are 2?
  uint32_t cellVoltages[numCells] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // convert to [2][6] for 2 batteries?
  uint16_t currentDraw = {20};
  uint16_t totalVoltage = {12};
  const char *error = ""; // TODO: change to string array so the warnings can be listed off
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
// PreCharge PreCharger();

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
constexpr uint8_t displayUpdateHz = 1;
uint16_t screenWidth;
uint16_t screenHeight;
BMSDataStruct BMSData;
BMSDataStruct LastBMSData = {};

/* ======= Declare Functions ======= */

void getBMSData();
void setLEDStripColour(LEDStripColourEnum colour);
void refreshDisplay();

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
#if DEBUG_ENABLED
  if (!JKMessenger.begin())
  {
    Serial.println("No BMS connection");
  }
#endif

  setLEDStripColour(MAGENTA_STARTING_CONFLICT_ERROR);

  // initialise TFT
  tft.begin();
  tft.setRotation(3);
  screenWidth = tft.width();
  screenHeight = tft.height();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(1);

  // divide screen into 6 regions
  tft.drawFastHLine(0, screenHeight / 2, screenWidth, ILI9341_WHITE);
  tft.drawFastVLine(screenWidth / 3, 0, screenHeight, ILI9341_WHITE);
  tft.drawFastVLine(screenWidth * 2 / 3, 0, screenHeight, ILI9341_WHITE);
}

void loop()
{
  LastBMSData = {};
  if (getDataFlag)
  {
    // getBMSData();
  }
  if (screenUpdateFlag)
  { // update flag driven by a hardware clock
    refreshDisplay();
#if DEBUG_ENABLED
    Serial.print( // print data to serial
        "Battery Life is: " + String(BMSData.batteryLife) + "; " +
        "MOS Status is: " + String(BMSData.MOSStatus) + "; " +
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
        "Total Voltage is: " + String(BMSData.totalVoltage) + "; " +
        "Error Flag is: " + String(BMSData.errorFlag) + "\n");
#endif
  }
  LastBMSData = BMSData;
}

void getBMSData()
{
  // TODO: link to BMS comms and assign values to struct
  JKMessenger.request_data();
  BMSData.batteryLife = JKMessenger.get_remaining_capacity_pct();
  for (size_t i = 0; i < numCells; i++)
  {
    BMSData.cellVoltages[i] = JKMessenger.get_cell_voltage_mV()->cellVoltage;
    // TODO: fix and iterate through retrieved values
  }
  BMSData.currentDraw = JKMessenger.get_current_dA();
  BMSData.MOSStatus[0] = JKMessenger.get_status_flags()->charging_MOS_status;
  BMSData.MOSStatus[1] = JKMessenger.get_status_flags()->discharge_MOS_status;
  BMSData.packTemp = JKMessenger.get_battery_temp_dC();
  BMSData.totalVoltage = JKMessenger.get_total_voltage_mV();
  for (size_t i = 0; i < 13; i++)
  {
    //  BMSData.error = strcat(BMSData.error, JKMessenger.get_warning_flags()->); // iterate struct fields
  }

  // error checking
  if (BMSData.error != "")
  {
    noInterrupts();
#if DISABLE_DISCHARGE_ON_ERROR
    JKMessenger.setMOS_state(false, false);
#endif
    tft.setCursor(screenHeight / 2, 0);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(3);
    tft.println("BMS Error Detected: "); // TODO: change to list off errors
    interrupts();
  }
  else
  {
  }
}

void refreshDisplay()
{
  tft.setTextSize(1);
  if (LastBMSData.totalVoltage != BMSData.totalVoltage)
  {
    tft.setCursor(2, 2);
    tft.println("Total Voltage: " + String(BMSData.totalVoltage) + "mV");
  }
  if (LastBMSData.packTemp != BMSData.packTemp)
  {
    // TODO: check temp range and change text colour to match
    tft.setCursor(screenWidth / 3, 2);
    tft.println("Pack Temp: " + String(BMSData.packTemp) + "°C");
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // reset text colour
  }
  if (LastBMSData.currentDraw != BMSData.currentDraw)
  {
    tft.setCursor(screenWidth * 2 / 3, 2);
    tft.println("Current Draw: " + String(BMSData.currentDraw) + "A");
  }
  if (LastBMSData.MOSStatus != BMSData.MOSStatus)
  {
    tft.setCursor(2, screenHeight / 2 + 2);
    tft.println("MOS Charge is " + BMSData.MOSStatus[0] ? "Disabled" : "Enabled");
    tft.println("MOS Discharge is " + BMSData.MOSStatus[1] ? "Disabled" : "Enabled");
  }
  if (LastBMSData.cellVoltages != BMSData.cellVoltages)
  {
    tft.setCursor(screenWidth / 3 + 2, screenHeight / 2 + 2);
    // show cell voltages in two blocks of six values
    // v v v    v v v
    // v v v    v v v
  }
  if (LastBMSData.batteryLife != BMSData.batteryLife)
  {
    tft.setCursor(screenWidth * 2 / 3 + 2, screenHeight / 2 + 2);
    tft.println("Battery remaining: " + String(BMSData.batteryLife) + "%");
  }
}

void setLEDStripColour(LEDStripColourEnum colour)
{
  // (tell ATTiny to?) set LEDstrip colour to colour value
}

// TODO: hardware timer interrupts for pulling data (?Hz) and refreshing the screen (1Hz)