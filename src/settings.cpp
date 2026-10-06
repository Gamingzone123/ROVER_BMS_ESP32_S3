#include "settings.h"

volatile bool screenUpdateFlag = false;
volatile bool getDataFlag = false;
volatile bool killFlag = false;
volatile bool encoderFlag = false;
volatile bool encoderTimeoutFlag = false;

hw_timer_t *dataTimer = NULL;
hw_timer_t *screenTimer = NULL;
hw_timer_t *encoderTimer = NULL;

uint8_t passcodeAttempt[passLength] = {};
volatile ulong lastEncoderChangeus = 0;
volatile bool encoderActive = false;
volatile bool encoderDirection = -1;
volatile EncoderModeEnum encoderMode = MODE_IDLE;
uint8_t passcodeIndex = 0;
uint8_t encoderState = 0;
SelectedMOSEnum selectedMOS = SEL_CHARGE;
bool MOSSwitchSelected = false;
volatile bool codeScreenTriggered = false;
volatile bool encoderMovedSinceStageStart = false;

/* Gray-code table: index = (prevState << 2) | newState, where each state is
 a 2-bit (CLK<<1 | DT) snapshot. Value is the direction of a valid single
 step (+1 / -1), or 0 if this transition can't happen from clean contacts
 (i.e. it's bounce/noise) or if nothing actually changed*/
const int8_t QUAD_TABLE[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

// Last known (CLK, DT) pair, packed the same way as QUAD_TABLE's index
volatile uint8_t encoderPrevState = 0;

JikongMessenger JKMessenger(&Serial2, BMS_COMMS_TIMEOUT_ms, numCells);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
const uint16_t screenWidth = tft.height();
const uint16_t screenHeight = tft.width();
const uint16_t dotY = screenHeight - 30;
const uint16_t spacing = screenWidth / (passLength + 1);
BMSDataStruct BMSData;
BMSDataStruct LastBMSData;