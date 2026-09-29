#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>

void beginDisplay();
void segmentDisplay();
void refreshDisplay();
void drawPasscodeDigit();
void displayBMSError(const std::string &error);

#endif