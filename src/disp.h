// disp.h - Display functions for vent_DEW LVGL version
#ifndef DISP_H
#define DISP_H

#include <TFT_eSPI.h>
#include "config.h"

extern TFT_eSPI tft;

void initDisplay();
void drawKOPButton();
void createKOPButton();
void updateTimeDisplay();

#endif // DISP_H
