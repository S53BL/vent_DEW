// config.h - Configuration for vent_DEW TFT_eSPI version
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// UI colors (from vent_REW config.h) - converted to 16-bit RGB565 for TFT_eSPI
#define BG_COLOR        0x3186  // 0x303030 -> approx
#define BTN_BLUE        0x0373  // 0x0066CC -> approx
#define BTN_GREEN       0x2646  // 0x00CC66 -> approx

#define BTN_RADIUS      10

#define BTN_EXT_COLOR       0x1C39  // 0x1C398E -> 0x1C39
#define BTN_TIME_WIFI_COLOR 0x2D2D  // 0x364153 -> approx
#define BTN_WC_COLOR        0x7A20  // 0x7B3306 -> 0x7A20
#define BTN_UT_COLOR        0x3548  // 0x35530E -> 0x3548
#define BTN_KOP_COLOR       0x024E  // 0x024A70 -> 0x024E
#define BTN_DS_COLOR        0x70E6  // 0x721378 -> 0x70E6
#define BTN_OPEN_COLOR      0xC080  // 0xC11007 -> 0xC080
#define BTN_CLOSED_COLOR    0x5E45  // 0x5EA529 -> 0x5E45

#define EXT_COLOR 0x1C39
#define TIME_WIFI_COLOR 0x2D2D
#define WC_COLOR 0x7A20
#define UT_COLOR 0x3548
#define KOP_COLOR 0x024E
#define DS_COLOR 0x70E6
#define WINDOW_OPEN_COLOR 0xC080  // Red
#define WINDOW_CLOSED_COLOR 0x5E45  // Green
#define LIGHT_YELLOW 0xD4E6  // 0xD0872E -> approx

// EXT gradient colors based on lux - converted to 16-bit
#define EXT_DARK 0x0842
#define EXT_NIGHT 0x0F22
#define EXT_TWILIGHT 0x1C4A
#define EXT_CLOUDY 0x2C9F
#define EXT_DAY 0x3AFF

// UI dimensions
#define CARD_RADIUS 12
#define CARD_MARGIN 8
#define ICON_SIZE 32

// Fonts mapped to TFT_eSPI built-in fonts
#define FONT_12 1  // tft.setTextFont(1) - 6x8
#define FONT_14 2  // tft.setTextFont(2) - 12x16
#define FONT_16 2  // tft.setTextFont(2) - 12x16
#define FONT_20 4  // tft.setTextFont(4) - 26x32
#define FONT_22 4  // tft.setTextFont(4) - 26x32
#define FONT_24 4  // tft.setTextFont(4) - 26x32
#define FONT_28 6  // tft.setTextFont(6) - 48x64

// NTP constants
#define NTP_UPDATE_INTERVAL 1800000  // 30 minutes
#define NTP_SERVER_COUNT 3

// NTP servers
extern const char* ntpServers[];

// Timezone string for Central European Time with DST
#define TZ_STRING "CET-1CEST,M3.5.0,M10.5.0/3"

#endif // CONFIG_H
