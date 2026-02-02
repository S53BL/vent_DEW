#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"
#include "disp.h"
#include <lvgl.h>

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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 LVGL vent_DEW");

  // Initialize display with LVGL
  initDisplay();

  // Setup WiFi
  Serial.println("Setting up WiFi...");
  bool wifiConnected = setupWiFi();
  Serial.println("WiFi setup complete");
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
