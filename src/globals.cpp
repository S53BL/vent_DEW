// globals.cpp - Global variables definitions for vent_DEW
#include "globals.h"

// NTP servers definition
const char* ntpServers[] = {"pool.ntp.org", "time.nist.gov", "time.google.com"};

// Global variables definitions
DEWData dewData;
Timezone myTZ;
bool timeSynced = false;

// WiFi variables
String wifiSSID = "";
bool connection_ok = false;
bool wifi_error = false;