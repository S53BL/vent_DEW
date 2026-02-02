#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "wifi_config.h"
#include "disp.h"
#include <lvgl.h>

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

DEWData dewData;

// HTTP Server
AsyncWebServer server(80);

// WiFi variables
String wifiSSID = "";
bool connection_ok = false;
bool wifi_error = false;

// WiFi constants
#define WIFI_CHECK_INTERVAL 30000  // 30 seconds
#define WIFI_RETRY_COUNT 3
#define WIFI_FIXED_DELAY 5000  // 5 seconds

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

        request->send(200, "application/json", "{\"status\":\"OK\"}");
    });

    // Heartbeat endpoint
    server.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "pong");
    });

    Serial.println("HTTP:Starting server");
    server.begin();
    Serial.println("HTTP:Server started on port 80");
    return true;
}

void setup() {
  Serial.begin(115200);

  // Wait 3 seconds to allow user to see serial output
  Serial.println("\n\n=== ESP32-S3 LVGL vent_DEW ===");
  Serial.println("Waiting 3 seconds for serial monitor...");
  delay(3000);
  Serial.println("Starting setup...");

  // Initialize display with LVGL
  Serial.println("Initializing display...");
  initDisplay();
  Serial.println("Display initialized");

  // Setup WiFi
  Serial.println("Setting up WiFi...");
  bool wifiConnected = setupWiFi();
  Serial.println("WiFi setup complete");

  // Setup HTTP server
  if (wifiConnected) {
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

  // Periodic status output every 3 seconds
  static unsigned long lastStatusOutput = 0;
  if (now - lastStatusOutput > 3000) {
      lastStatusOutput = now;
      Serial.printf("Status - millis: %lu, heap: %d, WiFi: %s\n",
                    now,
                    ESP.getFreeHeap(),
                    WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
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

  // Handle LVGL tasks
  lv_timer_handler();
  delay(1);
}
