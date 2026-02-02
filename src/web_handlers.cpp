// web_handlers.cpp - Web request handlers implementation for vent_DEW
#include "web_handlers.h"
#include "html.h"
#include <ESPAsyncWebServer.h>
#include <SD_MMC.h>
#include <vector>
#include <algorithm>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_clk.h>
#include <freertos/task.h>

// HTML template definitions
const char* HTML_ROOT = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>DEW - Ventilacijska enota</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
            text-align: center;
        }
        h1 {
            color: white;
            margin-bottom: 40px;
        }
        .nav {
            margin: 40px 0;
        }
        .nav a {
            color: #4da6ff;
            text-decoration: none;
            padding: 15px 30px;
            border: 2px solid #4da6ff;
            border-radius: 8px;
            font-size: 18px;
            display: inline-block;
        }
        .nav a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>DEW - Ventilacijska enota</h1>
    <div class="nav">
        <a href="/sd-list">SD datoteke</a>
    </div>
</body>
</html>)rawliteral";

const char* HTML_SD_LIST = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>DEW - SD datoteke</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
        }
        .content {
            max-width: 1200px;
            margin: 20px auto;
            background: #1a1a1a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #333;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background: #1a1a1a;
            border: 1px solid #333;
        }
        th, td {
            padding: 12px 15px;
            text-align: left;
            border: 1px solid #333;
        }
        th {
            background: #2a2a2a;
            color: white;
            font-weight: bold;
        }
        .scrollable {
            overflow-y: auto;
            max-height: 70vh;
        }
        .error {
            color: #ff4444;
            font-weight: bold;
            text-align: center;
            padding: 20px;
        }
        .back {
            text-align: center;
            margin-top: 20px;
        }
        .back a {
            color: #4da6ff;
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid #4da6ff;
            border-radius: 5px;
        }
        .back a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>SD datoteke</h1>
    <div class="content">
        <div class="scrollable">%s</div>
    </div>
    <div class="back">
        <a href="/">Nazaj na začetno stran</a>
    </div>
</body>
</html>)rawliteral";

// Helper function to format bytes to human readable format
String formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else return String(bytes / (1024.0 * 1024.0), 1) + " MB";
}

// Helper function to get SD card type string
String getCardTypeString(uint8_t cardType) {
    switch (cardType) {
        case CARD_NONE: return "Ni kartice";
        case CARD_MMC: return "MMC";
        case CARD_SD: return "SD";
        case CARD_SDHC: return "SDHC";
        case CARD_UNKNOWN: return "Neznano";
        default: return "Neznano";
    }
}

void handleRoot(AsyncWebServerRequest *request) {
    Serial.printf("WEB: %s %s\n", request->methodToString(), request->url().c_str());

    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>DEW - Ventilacijska enota</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
            margin-bottom: 30px;
        }
        .content {
            max-width: 1000px;
            margin: 0 auto 40px auto;
            background: #1a1a1a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #333;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background: #1a1a1a;
            border: 1px solid #333;
            margin-bottom: 20px;
        }
        th, td {
            padding: 10px 12px;
            text-align: left;
            border: 1px solid #333;
        }
        th {
            background: #2a2a2a;
            color: white;
            font-weight: bold;
            width: 30%;
        }
        .section-title {
            color: #4da6ff;
            font-size: 18px;
            margin: 20px 0 10px 0;
            padding-bottom: 5px;
            border-bottom: 1px solid #4da6ff;
        }
        .nav {
            text-align: center;
            margin-top: 30px;
        }
        .nav a {
            color: #4da6ff;
            text-decoration: none;
            padding: 15px 30px;
            border: 2px solid #4da6ff;
            border-radius: 8px;
            font-size: 18px;
            display: inline-block;
        }
        .nav a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>DEW - Sistemska diagnostika</h1>
    <div class="content">
)rawliteral";

    // RAM / Heap section
    html += "<div class='section-title'>RAM / Heap pomnilnik</div>";
    html += "<table>";
    html += "<tr><th>Prosti heap</th><td>" + formatBytes(esp_get_free_heap_size()) + "</td></tr>";
    html += "<tr><th>Prosti interni heap</th><td>" + formatBytes(esp_get_free_internal_heap_size()) + "</td></tr>";
    html += "<tr><th>Najmanjša prosta vrednost</th><td>" + formatBytes(esp_get_minimum_free_heap_size()) + "</td></tr>";
    html += "<tr><th>8-bit heap</th><td>" + formatBytes(heap_caps_get_free_size(MALLOC_CAP_8BIT)) + "</td></tr>";
    html += "<tr><th>PSRAM heap</th><td>" + formatBytes(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)) + "</td></tr>";
    html += "</table>";

    // Flash memory section
    html += "<div class='section-title'>Flash pomnilnik</div>";
    html += "<table>";
    html += "<tr><th>Velikost flash čipa</th><td>" + formatBytes(ESP.getFlashChipSize()) + "</td></tr>";
    html += "<tr><th>Velikost programa</th><td>" + formatBytes(ESP.getSketchSize()) + "</td></tr>";
    html += "<tr><th>Prostor za OTA</th><td>" + formatBytes(ESP.getFreeSketchSpace()) + "</td></tr>";
    html += "<tr><th>Hitrost flash čipa</th><td>" + String(ESP.getFlashChipSpeed() / 1000000.0, 1) + " MHz</td></tr>";
    html += "</table>";

    // SD card section
    html += "<div class='section-title'>SD kartica</div>";
    html += "<table>";
    if (SD_MMC.cardSize() > 0) {
        html += "<tr><th>Skupna velikost</th><td>" + formatBytes(SD_MMC.cardSize()) + "</td></tr>";
        html += "<tr><th>Uporabna velikost</th><td>" + formatBytes(SD_MMC.totalBytes()) + "</td></tr>";
        html += "<tr><th>Zasedena velikost</th><td>" + formatBytes(SD_MMC.usedBytes()) + "</td></tr>";
        html += "<tr><th>Tip kartice</th><td>" + getCardTypeString(SD_MMC.cardType()) + "</td></tr>";
    } else {
        html += "<tr><td colspan='2' style='text-align: center; color: #ff4444;'>SD kartica ni inicializirana</td></tr>";
    }
    html += "</table>";

    // PSRAM section
    html += "<div class='section-title'>PSRAM</div>";
    html += "<table>";
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (psram_free > 0) {
        html += "<tr><th>Prosta PSRAM</th><td>" + formatBytes(psram_free) + "</td></tr>";
        // Try to estimate total PSRAM size
        size_t psram_total = psram_free + heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
        html += "<tr><th>Skupna PSRAM (ocena)</th><td>" + formatBytes(psram_total) + "</td></tr>";
    } else {
        html += "<tr><td colspan='2' style='text-align: center; color: #888;'>PSRAM ni na voljo</td></tr>";
    }
    html += "</table>";

    // Time / Uptime section
    html += "<div class='section-title'>Čas / Uptime</div>";
    html += "<table>";
    html += "<tr><th>Uptime (millis)</th><td>" + String(millis()) + " ms</td></tr>";
    html += "<tr><th>Uptime (esp_timer)</th><td>" + String(esp_timer_get_time() / 1000) + " ms</td></tr>";
    html += "<tr><th>Uptime sekund</th><td>" + String(millis() / 1000) + " s</td></tr>";
    html += "</table>";

    // WiFi section
    html += "<div class='section-title'>WiFi</div>";
    html += "<table>";
    if (WiFi.status() == WL_CONNECTED) {
        html += "<tr><th>Signal (RSSI)</th><td>" + String(WiFi.RSSI()) + " dBm</td></tr>";
        html += "<tr><th>SSID</th><td>" + WiFi.SSID() + "</td></tr>";
        html += "<tr><th>IP naslov</th><td>" + WiFi.localIP().toString() + "</td></tr>";
    } else {
        html += "<tr><td colspan='2' style='text-align: center; color: #ff4444;'>WiFi ni povezan</td></tr>";
    }
    html += "</table>";

    // CPU section
    html += "<div class='section-title'>CPU</div>";
    html += "<table>";
    html += "<tr><th>CPU frekvenca</th><td>" + String(esp_clk_cpu_freq() / 1000000.0, 1) + " MHz</td></tr>";
    html += "</table>";

    // Stack usage section
    html += "<div class='section-title'>Stack usage</div>";
    html += "<table>";
    html += "<tr><th>Stack high water mark</th><td>" + String(uxTaskGetStackHighWaterMark(NULL)) + " besed</td></tr>";
    html += "</table>";

    // Close content and add navigation
    html += "</div>";
    html += "<div class='nav'>";
    html += "<a href='/sd-list'>SD datoteke</a>";
    html += "</div>";
    html += "</body></html>";

    request->send(200, "text/html", html);
}

struct FileInfo {
    String name;
    size_t size;
    String dateStr; // Simple day counter for sorting
    String mod;     // Display date
};

void handleSDList(AsyncWebServerRequest *request) {
    Serial.printf("WEB: %s %s\n", request->methodToString(), request->url().c_str());

    // Check if SD card is available by trying to open root
    File root = SD_MMC.open("/");
    if (!root) {
        Serial.printf("WEB: Response 503\n");
        request->send(503, "text/html", "<h1>SD ni na voljo</h1><a href='/'>Nazaj</a>");
        return;
    }
    root.close();

    String tableContent = "<table><tr><th>Ime</th><th>Velikost (bytes)</th><th>Zadnja sprememba</th></tr>";
    tableContent.reserve(30000);

    std::vector<FileInfo> files;

    root = SD_MMC.open("/");
    if (!root) {
        tableContent += "<tr><td colspan=\"3\" class=\"error\">Napaka pri odpiranju SD</td></tr>";
    } else {
        Serial.printf("Free heap before loop: %d\n", ESP.getFreeHeap());
        File entry = root.openNextFile();
        int fileCount = 0;
        while (entry && fileCount < 200) {
            if (!entry.isDirectory()) {
                String name = entry.name();
                Serial.printf("Processing file: %s\n", name.c_str());
                if (name.startsWith("dew_data_")) {
                    size_t size = entry.size();
                    // Extract date part from filename (dew_data_123.csv -> 123)
                    String dateStr = name.substring(9, name.lastIndexOf('.'));
                    String mod = "Dan " + dateStr; // Simple display since no NTP date

                    files.push_back({name, size, dateStr, mod});
                    fileCount++;
                }
            }
            entry.close();
            entry = root.openNextFile();
        }
        Serial.printf("Free heap after loop: %d\n", ESP.getFreeHeap());
        root.close();

        // Sort files by date (newest first) - dateStr is the day counter
        std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.dateStr.toInt() > b.dateStr.toInt(); // Descending order
        });

        // Generate HTML rows from sorted files
        for (const auto& file : files) {
            String row = "<tr><td>" + file.name + "</td><td>" + String(file.size) + "</td><td>" + file.mod + "</td></tr>";
            tableContent += row;
        }
    }

    tableContent += "</table>";

    char* htmlBuffer = (char*)malloc(35000);
    if (htmlBuffer) {
        snprintf(htmlBuffer, 35000, HTML_SD_LIST, tableContent.c_str());
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", htmlBuffer);
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
        free(htmlBuffer);
    } else {
        request->send(500, "text/plain", "Memory allocation failed");
    }
}