#include <Arduino.h>
#include "disp.h"
#include <lvgl.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 LVGL vent_DEW");

  // Initialize display with LVGL
  initDisplay();
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();
  delay(1);
}
