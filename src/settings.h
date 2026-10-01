#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <Jikong_Handler.h>
#include <string>

/* ======= Compiler Switches ======= */
#define DEBUG_ENABLED 1              // for debugging with a PC
#define NO_BMS 1                     // for testing without the BMS unit
#define NO_ATTiny 1                  // for testing without ATTiny for LED strip control
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

// Free / Spare: GPIO pins 2, 33, 37, 47, 48

/* ======= Application Settings ======= */
constexpr uint16_t BMS_COMMS_TIMEOUT_ms = 1000;
constexpr uint8_t numCells = 12;
constexpr uint8_t cellsPBattery = 6;
constexpr uint8_t numBatteries = 2;
constexpr uint8_t MOSPassword[] = {15, 7, 20};
constexpr uint8_t passLength = sizeof(MOSPassword) / sizeof(MOSPassword[0]); // compute num elements in array
constexpr uint32_t ENCODER_DIGIT_DWELL_ms = 1000;
constexpr uint32_t ENCODER_ATTEMPT_TIMEOUT_ms = ENCODER_DIGIT_DWELL_ms * passLength + 1500;
constexpr uint8_t ENCODER_ACTIVE_DISPLAY_UPDATE_HZ = 10;
constexpr uint8_t GET_DATA_RATE_HZ = 2;
constexpr uint8_t DISPLAY_UPDATE_HZ = 1;

/* ======= Data Types ======= */
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

enum EncoderModeEnum
{
    MODE_IDLE,
    MODE_SELECT_MOS,
    MODE_PASSCODE_ENTRY
};

enum SelectedMOSEnum
{
    SEL_CHARGE,
    SEL_DISCHARGE
};

/* ======= Runtime State ======= */
// Interrupt Flags
extern volatile bool screenUpdateFlag;
extern volatile bool getDataFlag;
extern volatile bool killFlag;
extern volatile bool encoderFlag;
extern volatile bool encoderTimeoutFlag;

// Hardware timers
extern hw_timer_t *dataTimer;
extern hw_timer_t *screenTimer;
extern hw_timer_t *encoderTimer;

// Encoder interaction state.
extern uint8_t passcodeAttempt[passLength];
extern volatile ulong lastEncoderChangeus;
extern volatile bool encoderActive;
extern volatile bool encoderDirection;
extern volatile EncoderModeEnum encoderMode;
extern uint8_t passcodeIndex;
extern uint8_t encoderState;
extern SelectedMOSEnum selectedMOS;
extern bool MOSSwitchSelected;
extern volatile bool codeScreenTriggered;
extern volatile bool encoderMovedSinceStageStart;

// Display and BMS readings.
extern JikongMessenger JKMessenger;
extern Adafruit_ILI9341 tft;
extern const uint16_t screenWidth;
extern const uint16_t screenHeight;
extern BMSDataStruct BMSData;
extern BMSDataStruct LastBMSData;
#endif