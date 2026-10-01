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

JikongMessenger JKMessenger(&Serial2, BMS_COMMS_TIMEOUT_ms, numCells);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
const uint16_t screenWidth = tft.height();
const uint16_t screenHeight = tft.width();
BMSDataStruct BMSData;
BMSDataStruct LastBMSData;