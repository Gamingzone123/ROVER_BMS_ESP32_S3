#ifndef BMS_DATA_H
#define BMS_DATA_H

#include "settings.h"

void getBMSData();
String getVerboseBMS();
#if !BMS_ENABLED
void getDummyBMS();
#endif

#endif