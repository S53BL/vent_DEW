// web_handlers.cpp - Web request handlers implementation for vent_DEW
#include "web_handlers.h"
#include "html.h"
#include <ESPAsyncWebServer.h>
#include <SD_MMC.h>
#include <vector>
#include <algorithm>

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

void handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", HTML_ROOT);
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