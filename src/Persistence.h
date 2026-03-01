#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#ifndef IS_NANO
  #include <ArduinoJson.h>
#endif

void saveState();
void loadState();
#ifndef IS_NANO
byte packDays(bool weekDays[]);
void unpackDays(byte packedDays, bool *weekDays);
JsonDocument getJsonData();
#endif

#endif
