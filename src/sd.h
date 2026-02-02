// sd.h - SD card module header for DEW
#ifndef SD_H
#define SD_H

#include <SD_MMC.h>

// Function declarations
bool initSD();
void saveDEWData();
String readFile(const char* path);

#endif // SD_H