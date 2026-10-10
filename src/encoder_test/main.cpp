#include <Arduino.h>

/* ======= Pins / settings (copied from settings.h) ======= */
constexpr uint8_t KY040_CLK = 3;
constexpr uint8_t KY040_DT = 15;
constexpr uint8_t KY040_SW = 16;

constexpr uint8_t MOSPassword[] = {15, 7, 20};
constexpr uint8_t passLength = sizeof(MOSPassword) / sizeof(MOSPassword[0]);
constexpr uint32_t ENCODER_DIGIT_DWELL_ms = 2500;
constexpr uint32_t ENCODER_ATTEMPT_TIMEOUT_ms = 10000;

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

/* ======= State ======= */
volatile bool encoderFlag = false;
volatile bool encoderActive = false;
volatile bool encoderMovedSinceStageStart = false;
volatile bool encoderDirection = CLOCKWISE;
volatile uint32_t lastEncoderChangeus = 0; // 32-bit: atomic on this core, wraps cleanly with micros()

// Test instrumentation: every accepted ISR edge vs every edge loop() actually handled.
// If these diverge on a fast spin, edges are being collapsed by the single encoderFlag.
volatile uint32_t isrEdgeCount = 0;
uint32_t handledCount = 0;

EncoderModeEnum encoderMode = MODE_IDLE;
uint8_t encoderState = 0;
uint8_t passcodeIndex = 0;
uint8_t passcodeAttempt[passLength] = {};
bool selectedMOSIsDischarge = false;

void encoder_CL_ISR();
void encoderHandler();
void incrementPasscode();
void checkPasscode();
void resetToIdle();

const char *modeName(EncoderModeEnum m)
{
    switch (m)
    {
    case MODE_IDLE:
        return "IDLE";
    case MODE_SELECT_MOS:
        return "SELECT_MOS";
    default:
        return "PASSCODE";
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1500); // let native USB CDC attach before the first print

    pinMode(KY040_CLK, INPUT);
    pinMode(KY040_DT, INPUT);
    pinMode(KY040_SW, INPUT);
    attachInterrupt(digitalPinToInterrupt(KY040_CLK), encoder_CL_ISR, RISING);

    Serial.printf("Encoder test ready. CLK=%d DT=%d (idle levels)\n",
                  digitalRead(KY040_CLK), digitalRead(KY040_DT));
}

void loop()
{
    // Snapshot once so both checks below see the same timestamp
    uint32_t sinceLastEdge = (uint32_t)(micros() - lastEncoderChangeus);

    // Inactivity timeout
    if (encoderActive && sinceLastEdge >= ENCODER_ATTEMPT_TIMEOUT_ms * 1000UL)
    {
        Serial.println("[TIMEOUT] no input -> back to IDLE");
        resetToIdle();
        return;
    }

    // Dwell confirmation (needs a genuine turn since the stage began)
    if (encoderActive && encoderMovedSinceStageStart &&
        sinceLastEdge >= ENCODER_DIGIT_DWELL_ms * 1000UL)
    {
        if (encoderMode == MODE_SELECT_MOS)
        {
            Serial.printf("[DWELL] selected %s -> PASSCODE entry\n",
                          selectedMOSIsDischarge ? "DISCHARGE" : "CHARGE");
            encoderMode = MODE_PASSCODE_ENTRY;
            passcodeIndex = 0;
            encoderState = 0;
            encoderMovedSinceStageStart = false; // digit 1 needs its own real turn
            lastEncoderChangeus = micros();
        }
        else if (encoderMode == MODE_PASSCODE_ENTRY)
        {
            incrementPasscode();
        }
    }

    if (encoderFlag)
    {
        encoderFlag = false; // cleared BEFORE handling so an edge arriving mid-print isn't wiped
        encoderHandler();
    }
}

void encoderHandler()
{
    handledCount++;

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
        encoderMode = MODE_SELECT_MOS;
        encoderState = 0;
        modulus = 2;
        Serial.println("[MODE] IDLE -> SELECT_MOS");
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

    if (encoderMode == MODE_SELECT_MOS)
    {
        selectedMOSIsDischarge = (encoderState == 1);
    }

    Serial.printf("[%s] %s  state=%u (shown %u)  edges=%lu handled=%lu\n",
                  modeName(encoderMode),
                  encoderDirection ? "CCW" : "CW ",
                  encoderState,
                  encoderMode == MODE_PASSCODE_ENTRY ? encoderState + 1 : encoderState,
                  (unsigned long)isrEdgeCount, (unsigned long)handledCount);
}

void incrementPasscode()
{
    passcodeAttempt[passcodeIndex] = encoderState + 1; // stored as 1-20
    Serial.printf("[DWELL] digit %u/%u confirmed = %u\n",
                  passcodeIndex + 1, passLength, passcodeAttempt[passcodeIndex]);
    passcodeIndex++;
    encoderState = 0;
    encoderMovedSinceStageStart = false;

    if (passcodeIndex >= passLength)
    {
        checkPasscode();
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
    Serial.println(correct ? "[PASSCODE] CORRECT (would toggle MOS)" : "[PASSCODE] WRONG");
    resetToIdle();
}

void resetToIdle()
{
    encoderActive = false;
    encoderMovedSinceStageStart = false;
    encoderMode = MODE_IDLE;
    encoderState = 0;
    passcodeIndex = 0;
    for (uint8_t i = 0; i < passLength; i++)
    {
        passcodeAttempt[i] = 0;
    }
    Serial.println("[IDLE] reset");
}

void IRAM_ATTR encoder_CL_ISR()
{
    encoderDirection = (digitalRead(KY040_DT) == HIGH) ? ANTICLOCKWISE : CLOCKWISE;
    lastEncoderChangeus = micros();
    isrEdgeCount++;

    encoderActive = true;
    encoderMovedSinceStageStart = true;
    encoderFlag = true;
}