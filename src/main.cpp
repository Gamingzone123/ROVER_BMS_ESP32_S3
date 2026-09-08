/* ======= Includes ======= */
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h> // SPI 0 and 1 are used by the board itself, SPI 2 is used for the ethernet, must use eth 3 for tft screen
#include <ArduinoJson.h>

/* ======= Compiler Switches ======= */
#define DEBUG_ENABLED 0              // compiler switch for debugging with a PC
#define DISABLE_DISCHARGE_ON_ERROR 0 // whether to disable rover power on BMS error

/* ======= Pin defs ======= */

/*preset SPI pins for Ethernet port are hardwired not changeable*/
#define ETH_RST 14  // GPIO9
#define ETH_INT 15  // GPIO10
#define ETH_MOSI 16 // GPIO11
#define ETH_MISO 17 // GPIO12
#define ETH_SCLK 18 // GPIO13
#define ETH_CS 19   // GPIO14

/*the SD card pins are also set in stone if it is in use*/
#define SD_CS 9    // GPIO4
#define SD_MISO 10 // GPIO5
#define SD_MOSI 11 // GPIO6
#define SD_CLK 12  // GPIO7

/*UART 1 for BMS comms*/
#define BMS_TX 23 // GPIO17
#define BMS_RX 24 // GPIO18

/*SPI pins for TFT screen uses SPI3 which must be routed via GPIO matrix, meaning they can be assigned to pretty much any  unused pins*/ // TODO: learn how to and implement this matrix stuff
// values are placeholders
#define TFT_RST 38  // GPIO33
#define TFT_MOSI 39 // GPIO34
#define TFT_DC 40   // GPIO35
#define TFT_SCLK 41 // GPIO36
#define TFT_CS 42   // GPIO37

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
  bool errorFlag;
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
  Serial2.begin(115200, /*TODO: figure out what to put here*/, BMS_RX, BMS_TX);
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
    if (BMSData.errorFlag)
    {
      noInterrupts();
#if DISABLE_DISCHARGE_ON_ERROR
      // disable discharge on precharge module
#endif
      tft.setCursor(screenHeight / 2, 0);
      tft.setTextColor(ST7735_RED);
      tft.setTextSize(3);
      tft.println("BMS Error Detected");
      interrupts();
    }
  }
  if (screenUpdateFlag)
  { // update flag driven by a hardware clock
    refreshDisplay();
#if DEBUG_ENABLED
    Serial.print(
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
  }
  if (LastBMSData.packTemp != BMSData.packTemp)
  {
  }
  if (LastBMSData.currentDraw != BMSData.currentDraw)
  {
  }
  if (LastBMSData.MOSStatus != BMSData.MOSStatus)
  {
    tft.setCursor(screenWidth / 3 + 2, screenHeight / 2 + 2);
    tft.println("Charge is " + BMSData.MOSStatus[0] ? "Disabled" : "Enabled");
    tft.println("Discharge is " + BMSData.MOSStatus[1] ? "Disabled" : "Enabled");
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