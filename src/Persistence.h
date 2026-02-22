#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <ArduinoJson.h>

void saveState();
void loadState();
byte packDays(bool weekDays[]);
void unpackDays(byte packedDays, bool *weekDays);
JsonDocument getJsonData();

#endif
