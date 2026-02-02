// sd.cpp - SD card module implementation for DEW
#include "sd.h"
#include "globals.h"  // For dewData access

// SD card pins for ESP32-S3-LCD-1.3 (from Arduino demo)
#define SDMMC_CLK 21  // GPIO21
#define SDMMC_CMD 18  // GPIO18
#define SDMMC_D0  16  // GPIO16

bool initSD() {
    Serial.println("SD:Setting pins for ESP32-S3-LCD-1.3");
    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);

    Serial.println("SD:Initializing SD card...");
    if (!SD_MMC.begin("/sdcard", true, true, 8000000)) {
        Serial.println("SD:Initialization failed!");
        return false;
    }

    Serial.println("SD:Card initialized successfully");
    return true;
}

void saveDEWData() {
    static String currentDate = "";
    static String currentFile = "";

    // Get current date
    String today = String(millis() / 86400000);  // Simple day counter since no NTP

    // Create new file if date changed
    if (currentDate != today) {
        currentDate = today;
        currentFile = "/dew_data_" + currentDate + ".csv";

        // Create file with header if it doesn't exist
        if (!SD_MMC.exists(currentFile.c_str())) {
            File file = SD_MMC.open(currentFile.c_str(), FILE_WRITE);
            if (file) {
                file.println("Timestamp,Room,Fan,Countdown,Temp,Humidity,Pressure,Error");
                file.close();
                Serial.println("SD:Created new DEW data file: " + currentFile);
            } else {
                Serial.println("SD:Failed to create DEW data file");
                return;
            }
        }
    }

    // Append data to file
    File file = SD_MMC.open(currentFile.c_str(), FILE_APPEND);
    if (!file) {
        Serial.println("SD:Failed to open file for append: " + currentFile);
        return;
    }

    // Format: timestamp,room,fan,countdown,temp,humidity,pressure,error
    char line[128];
    sprintf(line, "%lu,%s,%d,%d,%.1f,%.1f,%.1f,%d",
            dewData.lastUpdate,
            dewData.room.c_str(),
            dewData.fan,
            dewData.countdown,
            dewData.temp,
            dewData.humidity,
            dewData.pressure,
            dewData.error);

    file.println(line);
    file.close();

    Serial.println("SD:DEW data saved to " + currentFile);
}

String readFile(const char* path) {
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial.println("SD:File open failed: " + String(path));
        return "";
    }
    String s;
    while (f.available()) s += (char)f.read();
    f.close();
    return s;
}