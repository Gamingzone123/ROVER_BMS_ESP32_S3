#include "Display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include "settings.h"

void beginDisplay()
{
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(1);
}

void segmentDisplay() // divide screen into 6 regions
{
    tft.fillScreen(ILI9341_BLACK);
    tft.drawFastHLine(0, screenHeight / 2, screenWidth, ILI9341_WHITE);
    tft.drawFastVLine(screenWidth / 3, 0, screenHeight, ILI9341_WHITE);
    tft.drawFastVLine(screenWidth * 2 / 3, 0, screenHeight, ILI9341_WHITE);
}

void displayBMSError(const std::string &error)
{
    tft.setCursor(screenHeight / 2, 0);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.setTextSize(3);
    tft.setTextWrap(1);
    tft.print("BMS Errors Detected: " + String(error.c_str()) + "\n"); // list errors
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
        auto printSection = [&](const String &label, const String &value, uint16_t left, uint16_t top,
                                uint16_t valueSize = 4, uint16_t valueColor = ILI9341_WHITE,
                                uint16_t labelColor = ILI9341_WHITE)
        {
            tft.setTextColor(labelColor, ILI9341_BLACK);
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
        bool mosSectionNeedsRedraw = (LastBMSData.MOSStatus[0] != BMSData.MOSStatus[0]) ||
                                     (LastBMSData.MOSStatus[1] != BMSData.MOSStatus[1]) ||
                                     (encoderMode == MODE_SELECT_MOS);
        if (mosSectionNeedsRedraw)
        {
            uint16_t chargeLabelColor = (encoderMode == MODE_SELECT_MOS && selectedMOS == SEL_CHARGE) ? ILI9341_YELLOW : ILI9341_WHITE;
            uint16_t dischargeLabelColor = (encoderMode == MODE_SELECT_MOS && selectedMOS == SEL_DISCHARGE) ? ILI9341_YELLOW : ILI9341_WHITE;

            printSection("Charge MOS", BMSData.MOSStatus[0] ? "Enabled" : "Disabled", 0, rowHeight + 3, 2,
                         BMSData.MOSStatus[0] ? ILI9341_GREEN : ILI9341_RED, chargeLabelColor);
            printSection("Discharge MOS", BMSData.MOSStatus[1] ? "Enabled" : "Disabled", 0, rowHeight + 26 + 3, 2,
                         BMSData.MOSStatus[1] ? ILI9341_GREEN : ILI9341_RED, dischargeLabelColor);
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

void drawPasscodeDigit()
{
    if (passcodeIndex == 0)
    {
        timerWrite(encoderTimer, 0);
        if (!codeScreenTriggered)
        {
            tft.fillScreen(ILI9341_BLACK);
            codeScreenTriggered = true;
        }
    }

    // current encoder position so the user knows what they are entering
    String posText = String(encoderState + 1);
    int16_t x1, y1;
    uint16_t textW, textH;
    tft.setTextSize(8);
    tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
    tft.getTextBounds(posText, 0, 0, &x1, &y1, &textW, &textH);
    tft.setCursor((screenWidth - textW) / 2, screenHeight / 3);
    tft.println(posText);

    // Progress dots filled once that digit has been entered.
    const uint16_t dotRadius = 8;
    const uint16_t dotY = screenHeight - 30;
    const uint16_t spacing = screenWidth / (passLength + 1);

    for (uint8_t i = 0; i < passLength; i++)
    {
        uint16_t dotX = spacing * (i + 1);
        if (i < passcodeIndex)
        {
            tft.fillCircle(dotX, dotY, dotRadius, ILI9341_WHITE);
        }
        else
        {
            tft.drawCircle(dotX, dotY, dotRadius, ILI9341_WHITE);
        }
    }
}