/* ======= Includes ======= */
#include <Arduino.h>
#include <Adafruit_GFX.h>    // base adafruit graphic lib required by the tft
#include <Adafruit_ST7735.h> // library for the tft
#include <SPI.h>             // SPI 0 and 1 are used by the board itself, SPI 2 is used for the ethernet, must use eth 3 for tft screen
#include <ArduinoJson.h>
#include <stdint.h>

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
volatile bool screenUpdateFlag = false;
volatile bool getDataFlag = false;

/* ======= Structs, Enums & Type Defs*/

struct BMSDataStruct
{
  u_int32_t batteryLife;
  bool MOSStatus[2];       // {charge, discharge} both 0 or 1
  double packTemp;         // is this per battery, there are 2?
  double cellVoltages[12]; // convert to [2][6] for 2 batteries?
  double currentDraw;      // per battery?
  double totalVoltage;
  String error = ""; // TODO: change to string array so the warnings can be listed off
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

/* ======= Globals ======= */

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
const uint8_t displayUpdateHz = 1;
const uint8_t screenWidth;  // TODO: compute and assign a value based on tft.width() here
const uint8_t screenHeight; // same with height
BMSDataStruct BMSData;
BMSDataStruct LastBMSData;

/* ======= The Program =======*/

void setup()
{
#if DEBUG_ENABLED
  Serial.begin(115200);
#endif
  Serial2.begin(115200, SERIAL_8N1, JIKONG_RX, JIKONG_TX);
  // test precharge connection
  // test ethernet connection
  // test ROS2 connection
  // test ATTiny connection(?)
#if DEBUG_ENABLED
  if (!Serial2)
  {
    Serial.println("No BMS connection");
  }
#endif

  setLEDStripColour(MAGENTA_STARTING_CONFLICT_ERROR);

  // initialise TFT
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3); // TODO: need to check if this is correct, can be changed to fit hardware orientation
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);

  // divide screen into 6 regions
  tft.drawFastHLine(0, screenHeight / 2, screenWidth, ST7735_WHITE);
  tft.drawFastVLine(screenWidth / 3, 0, screenHeight, ST7735_WHITE);
  tft.drawFastVLine(screenWidth * 2 / 3, 0, screenHeight, ST7735_WHITE);
}

void loop()
{
  if (getDataFlag)
  {
    BMSData = getBMSData();
    if (BMSData.error != "")
    {
      noInterrupts();
#if DISABLE_DISCHARGE_ON_ERROR
      // disable discharge on precharge module
#endif
      tft.setCursor(screenHeight / 2, 0);
      tft.setTextColor(ST7735_RED);
      tft.setTextSize(3);
      tft.println("BMS Error Detected"); // TODO: change to list off errors
      interrupts();
    }
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
  // TODO: find a way to discard excess serial data from BMS
}

BMSDataStruct getBMSData()
{
  // TODO: link to BMS comms and assign values to struct
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
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // reset text colour
  }
  if (LastBMSData.currentDraw != BMSData.currentDraw)
  {
    tft.setCursor(screenWidth * 2 / 3, 2);
    tft.println("Current Draw: " + String(BMSData.currentDraw) + "A");
  }
  if (LastBMSData.MOSStatus != BMSData.MOSStatus)
  {
    tft.setCursor(screenWidth / 3 + 2, screenHeight / 2 + 2);
    tft.println("MOS Charge is " + BMSData.MOSStatus[0] ? "Disabled" : "Enabled");
    tft.println("MOS Discharge is " + BMSData.MOSStatus[1] ? "Disabled" : "Enabled");
  }
  if (LastBMSData.cellVoltages != BMSData.cellVoltages)
  {
  }
  if (LastBMSData.batteryLife != BMSData.batteryLife)
  {
  }
}

void setLEDStripColour(LEDStripColourEnum colour)
{
  // set LEDstrip colour to colour value
}

// TODO: hardware timer interrupts for pulling data (?Hz) and refreshing the screen (1Hz)