// globals.h - Global variables and structures for DEW
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <ezTime.h>

// DEW data structure
struct DEWData {
    String room = "";
    uint8_t fan = 0;
    uint32_t countdown = 0;
    float temp = 0.0f;
    float humidity = 0.0f;
    float pressure = 0.0f;
    uint8_t error = 0;
    uint32_t lastUpdate = 0;
};

extern DEWData dewData;
extern Timezone myTZ;
extern bool timeSynced;

// WiFi variables
extern String wifiSSID;
extern bool connection_ok;
extern bool wifi_error;

#endif // GLOBALS_H
