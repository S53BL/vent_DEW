#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ezTime.h>
#include "wifi_config.h"
#include "disp.h"
#include "globals.h"
#include "sd.h"
#include "web.h"
#include <lvgl.h>

// HTTP Server
AsyncWebServer server(80);

// WiFi constants
#define WIFI_CHECK_INTERVAL 30000  // 30 seconds
#define WIFI_RETRY_COUNT 3
#define WIFI_FIXED_DELAY 5000  // 5 seconds

// Extern declarations for global variables
extern String wifiSSID;
extern bool connection_ok;
extern bool wifi_error;

bool setupWiFi() {
    Serial.println("WiFi:Configuring static IP...");
    WiFi.config(localIP, gateway, subnet, dns);
    Serial.println("WiFi:Static IP configured");

    int numNetworks = sizeof(ssidList)/sizeof(ssidList[0]);
    Serial.printf("WiFi:Trying %d networks...\n", numNetworks);

    for (int i = 0; i < numNetworks; i++) {
        Serial.printf("WiFi:Trying network %d: %s\n", i+1, ssidList[i]);
        WiFi.begin(ssidList[i], passwordList[i]);

        unsigned long start = millis();
        int attempts = 0;
        while (millis() - start < 15000) {  // Increased timeout to 15s
            wl_status_t status = WiFi.status();
            Serial.printf("WiFi:Status=%d (attempt %d)\n", status, ++attempts);
            if (status == WL_CONNECTED) {
                wifiSSID = ssidList[i];
                Serial.printf("WiFi:Connected! IP=%s\n", WiFi.localIP().toString().c_str());
                connection_ok = true;
                wifi_error = false;
                return true;
            }
            delay(1000);  // Log every second
        }
        Serial.printf("WiFi:Network %s timeout\n", ssidList[i]);
        WiFi.disconnect();
        delay(1000);
    }
    Serial.println("WiFi:All networks failed");
    wifi_error = true;
    connection_ok = false;
    return false;
}

void setupNTP() {
    myTZ.setPosix(TZ_STRING);
    events();
    setInterval(NTP_UPDATE_INTERVAL / 1000);
    Serial.println("NTP: Timezone set to CET/CEST");
}

bool syncNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("NTP: WiFi not connected - cannot sync");
        timeSynced = false;
        return false;
    }

    Serial.println("NTP: Starting NTP sync with servers:");
    for (int i = 0; i < NTP_SERVER_COUNT; i++) {
        Serial.printf("NTP: Trying server %d: %s\n", i+1, ntpServers[i]);

        setServer(ntpServers[i]);
        updateNTP();
        delay(2000); // Wait for NTP response

        // Check if time is now valid (ezTime sets time after successful sync)
        if (myTZ.now() > 1609459200) { // Check if time is after 2021-01-01 (reasonable check)
            Serial.printf("NTP: SUCCESS with %s\n", ntpServers[i]);
            Serial.printf("NTP: Current time: %s\n", myTZ.dateTime().c_str());
            timeSynced = true;
            return true;
        } else {
            Serial.printf("NTP: FAILED with %s (invalid time)\n", ntpServers[i]);
        }

        delay(1000); // Small delay between attempts
    }

    Serial.println("NTP: All servers failed - time not synchronized");
    timeSynced = false;
    return false;
}

bool setupServer() {
    Serial.println("HTTP:Setting up server endpoints");

    // DEW update endpoint - receives data from CEW
    server.on("/api/dew-update", HTTP_POST, [](AsyncWebServerRequest *request){
        String body = request->arg("plain");
        Serial.println("HTTP:Received /api/dew-update body: " + body);

        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            Serial.println("HTTP:JSON parse error: " + String(error.c_str()));
            request->send(400, "application/json", "{\"status\":\"ERROR\",\"message\":\"Invalid JSON\"}");
            return;
        }

        // Parse DEW data
        dewData.room = doc["room"] | "";
        dewData.fan = doc["fan"] | 0;
        dewData.countdown = doc["countdown"] | 0;
        dewData.temp = doc["temp"] | 0.0f;
        dewData.humidity = doc["humidity"] | 0.0f;
        dewData.pressure = doc["pressure"] | 0.0f;  // Optional for KOP
        dewData.error = doc["error"] | 0;
        dewData.lastUpdate = millis();

        Serial.printf("HTTP:Parsed DEW data - room:%s fan:%d countdown:%d temp:%.1f hum:%.1f pres:%.1f err:%d\n",
                      dewData.room.c_str(), dewData.fan, dewData.countdown,
                      dewData.temp, dewData.humidity, dewData.pressure, dewData.error);

        // Save data to SD card
        saveDEWData();

        request->send(200, "application/json", "{\"status\":\"OK\"}");
    });

    // Heartbeat endpoint
    server.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "pong");
    });

    // Setup web interface endpoints
    setupWebEndpoints();

    Serial.println("HTTP:Starting server");
    server.begin();
    Serial.println("HTTP:Server started on port 80");
    return true;
}

void setup() {
  Serial.begin(115200);
  // 5-second delay after Serial.begin() to allow serial monitor to connect
  delay(5000);

  Serial.println("\n\n=== ESP32-S3 LVGL vent_DEW ===");
  Serial.println("Starting setup...");

  // Initialize display with LVGL
  Serial.println("Initializing display...");
  initDisplay();
  Serial.println("Display initialized");

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (!initSD()) {
    Serial.println("SD card initialization failed - continuing without SD logging");
  } else {
    Serial.println("SD card initialized successfully");
  }

  // Setup WiFi
  Serial.println("Setting up WiFi...");
  bool wifiConnected = setupWiFi();
  Serial.println("WiFi setup complete");

  if (wifiConnected) {
    Serial.println("Setting up NTP...");
    setupNTP();
    syncNTP();  // Try to sync NTP on startup
    Serial.println("NTP setup complete");

    Serial.println("Setting up HTTP server...");
    if (!setupServer()) {
      Serial.println("HTTP server setup failed");
    } else {
      Serial.println("HTTP server initialized");
      Serial.println("Ready to receive data from CEW");
    }
  } else {
    Serial.println("HTTP server not started - WiFi not connected");
  }

  Serial.println("Setup complete - entering main loop");
}

void loop() {
  uint32_t now = millis();

  // Periodic status output every 10 seconds
  static unsigned long lastStatusOutput = 0;
  if (now - lastStatusOutput > 10000) {
      lastStatusOutput = now;
      Serial.printf("myTZ %ld %s heap: %d\n",
                    myTZ.now(),
                    myTZ.dateTime("H:i:s").c_str(),
                    ESP.getFreeHeap());
  }

  // WiFi reconnect logic
  static unsigned long lastWiFiCheck = 0;
  if (now - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
      lastWiFiCheck = now;
      if (WiFi.status() != WL_CONNECTED) {
          connection_ok = false;
          Serial.println("WiFi:Disconnected - attempting reconnect");
          for (int attempt = 0; attempt < WIFI_RETRY_COUNT; attempt++) {
              if (setupWiFi()) {
                  wifi_error = false;
                  connection_ok = true;
                  break;
              }
              delay(WIFI_FIXED_DELAY);
          }
          if (WiFi.status() != WL_CONNECTED) {
              wifi_error = true;
              delay(30000);
          }
      }
  }

  // NTP update logic - retry if not synced or periodic update
  static unsigned long lastNTPUpdate = 0;
  if (WiFi.status() == WL_CONNECTED &&
      (!timeSynced || now - lastNTPUpdate > NTP_UPDATE_INTERVAL)) {
      lastNTPUpdate = now;
      syncNTP();
  }

  // Ensure lv_timer_handler every 5ms (like in vent_REW)
  static uint32_t lastLvgl = 0;
  if (now - lastLvgl >= 5) {
      lv_timer_handler();
      lastLvgl = now;
  }

  // Update time display every second
  static unsigned long lastTimeUpdate = 0;
  if (now - lastTimeUpdate >= 1000) {
      updateTimeDisplay();
      lastTimeUpdate = now;
  }

  delay(1);
}
