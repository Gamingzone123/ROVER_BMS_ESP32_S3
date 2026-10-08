#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <Jikong_Handler.h>
#include <string>
#include <ESP32Encoder.h>

/* ======= Compiler Switches ======= */
#define DEBUG_ENABLED 1  // for debugging with a PC
#define BMS_ENABLED 0    // set to 0 for testing without the BMS unit
#define ATTINY_ENABLED 0 // set to 0 for testing without ATTiny for LED strip control
#define ROS_ENABLED 0    // set to 0 for testing without MicroROS
#define SD_ENABLED 0     // set to 0 for testing without SD card
// TODO: implement SD card
#if SD_ENABLED
#define CARD_LOGGING_ENABLED 0 // set to 0 for teting without logging to SD card
// TODO: implement logging to sd card
#endif
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

/*I2C pins*/
constexpr uint8_t MAINSDA = 34;
constexpr uint8_t MAINSCL = 35;

/*IO Pins for comms with ATTiny85*/
constexpr uint8_t ATTINY_SDA = MAINSDA;
constexpr uint8_t ATTINY_SCL = MAINSCL;

/*Pins for use with rotary encoder*/
constexpr uint8_t KY040_CLK = 3;
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
constexpr uint32_t ENCODER_DIGIT_DWELL_ms = 2500;
constexpr uint32_t ENCODER_ATTEMPT_TIMEOUT_ms = ENCODER_DIGIT_DWELL_ms * passLength + 1500;
constexpr uint8_t ENCODER_ACTIVE_DISPLAY_UPDATE_HZ = 30;
constexpr uint8_t GET_DATA_RATE_HZ = 2;
constexpr uint8_t DISPLAY_UPDATE_HZ = 1;
constexpr uint8_t LED_UPDATE_RETRIES = 5;

constexpr uint8_t ATTINY_ADDR = 0x08; // I2C address for ATTiny

/* ======= Data Types ======= */
struct BMSDataStruct
{
    uint8_t batteryLife = 50;
    bool MOSStatus[2] = {0, 0}; // {charge, discharge} both 0 or 1
    int16_t packTemp = 25;      // is this per battery, there are 2?
    uint32_t cellVoltages[numCells] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    uint16_t currentDraw = 2;
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
constexpr uint16_t DIGIT_FIELD_W = 100;
constexpr uint16_t DIGIT_FIELD_H = 70;

// Display and BMS readings.
extern JikongMessenger JKMessenger;
extern Adafruit_ILI9341 tft;
extern const uint16_t screenWidth;
extern const uint16_t screenHeight;
constexpr uint16_t dotRadius = 8;
extern const uint16_t dotY;
extern const uint16_t spacing;
extern BMSDataStruct BMSData;
extern BMSDataStruct LastBMSData;
#endif