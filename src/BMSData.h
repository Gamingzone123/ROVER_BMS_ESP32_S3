#ifndef BMS_DATA_H
#define BMS_DATA_H

#include "settings.h"

void getBMSData();
String getVerboseBMS();
#if NO_BMS
void getDummyBMS();
#endif

#endif