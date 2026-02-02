/***
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * libhelix: https://github.com/pschatzmann/arduino-libhelix.git
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 */
#define MJPEG_FILENAME "/288_12fps.mjpeg"
#define FPS 12
#define MJPEG_BUFFER_SIZE (240 * 240 * 4 / 10)

#define SDMMC_D3 17  // SDMMC Data3 / SPI CS
#define SDMMC_CMD 18 // SDMMC CMD   / SPI MOSI
#define SDMMC_CLK 21 // SDMMC CLK   / SPI SCK
#define SDMMC_D0 16  // SDMMC Data0 / SPI MISO

#include <WiFi.h>
#include <FS.h>

#include <FFat.h>
#include <LittleFS.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPIFFS.h>

#include <TFT_eSPI.h>

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define LCD_BL 20
/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(38 /* DC */, 39 /* CS */, 40 /* SCK */, 41 /* MOSI */, GFX_NOT_DEFINED /* MISO */, FSPI /* spi_num */);
/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ST7789(bus, 42 /* RST */, 0 /* rotation */, true /* IPS */, 240 /* width */, 240 /* height */, 0 /* col offset 1 */, 80 /* row offset 1 */, 0 /* col offset 2 */, 0 /* row offset 2 */);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/
TFT_eSPI tft = TFT_eSPI(240,240); 
/* variables */
static int next_frame = 0;
static int skipped_frames = 0;
static unsigned long start_ms, curr_ms, next_frame_ms;
static unsigned long total_read_video_ms = 0;
static unsigned long total_decode_video_ms = 0;
static unsigned long total_show_video_ms = 0;

/* MJPEG Video */
#include "MjpegClass.h"
static MjpegClass mjpeg;

void PWM_Init()
{
  pinMode (LCD_BL,OUTPUT);
  digitalWrite(LCD_BL,HIGH);

  
  ledcSetup(1,1000,8);
  
  ledcAttachPin(LCD_BL,1);
}

void LCD_PWM(unsigned char i)
{
  ledcWrite(1,i);
  delay(10);
}

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw)
{

  unsigned long s = millis();
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  total_show_video_ms += millis() - s;
  return 1;
} /* drawMCU() */

void setup()
{
  // WiFi.mode(WIFI_OFF);
  // gfx->setRotation(0);
  Serial.begin(115200);
  LCD_PWM(255);
  // tft.begin();
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  Serial.println("Init display");
  if (!gfx->begin(40000000))
  {
    Serial.println("Init display failed!");
  }
  gfx->fillScreen(BLACK);
  // gfx->setRotation(4);
  
  tft.setRotation(5);
  // SPIClass spi = SPIClass(SPI2_HOST);
  // spi.begin(SDMMC_CLK, SDMMC_D0 /* MISO */, SDMMC_CMD /* MOSI */, SDMMC_D3 /* SS */);
  // if (!SD.begin(SDMMC_D3 /* SS */, spi, 40000000))

  pinMode(SDMMC_D3 /* CS */, OUTPUT);
  digitalWrite(SDMMC_D3 /* CS */, HIGH);
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true,8000000)) {
    Serial.println("Card Mount Failed");
    return;
  }

  // if (!SD_MMC.begin("/root", true, true,4000000)) /* 1-bit SD bus mode */
  // {
  //   Serial.println("ERROR: File system mount failed!");
  //   gfx->println("ERROR: File system mount failed!");
  // }
  else
  {
      Serial.println("Open MJPEG file: " MJPEG_FILENAME);
      gfx->println("Open MJPEG file: " MJPEG_FILENAME);
      // File vFile = LittleFS.open(MJPEG_FILENAME);
      // File vFile = SPIFFS.open(MJPEG_FILENAME);
      // File vFile = FFat.open(MJPEG_FILENAME);
      // File vFile = SD.open(MJPEG_FILENAME);
      File vFile = SD_MMC.open(MJPEG_FILENAME);
      if (!vFile || vFile.isDirectory())
      {
        Serial.println("ERROR: Failed to open " MJPEG_FILENAME " file for reading");
        gfx->println("ERROR: Failed to open " MJPEG_FILENAME " file for reading");
      }
      else
      {
        uint8_t *mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
        if (!mjpeg_buf)
        {
          Serial.println("mjpeg_buf malloc failed!");
        }
        else
        {
          mjpeg.setup(
              &vFile, mjpeg_buf, drawMCU, true /* useBigEndian */,
              0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
          start_ms = millis();
          curr_ms = millis();
          next_frame_ms = start_ms + (++next_frame * 1000 / FPS / 2);
          while (vFile.available() && mjpeg.readMjpegBuf()) // Read video
          {
            total_read_video_ms += millis() - curr_ms;
            curr_ms = millis();

            if (millis() < next_frame_ms) // check show frame or skip frame
            {
              // Play video
              mjpeg.drawJpg();
              total_decode_video_ms += millis() - curr_ms;
              curr_ms = millis();
            }
            else
            {
              ++skipped_frames;
              Serial.println("Skip frame");
            }

            while (millis() < next_frame_ms)
            {
              vTaskDelay(pdMS_TO_TICKS(1));
            }

            curr_ms = millis();
            next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
          }

#define CHART_MARGIN 64
#define LEGEND_A_COLOR 0x1BB6
#define LEGEND_B_COLOR 0xFBE1
#define LEGEND_C_COLOR 0x2D05
#define LEGEND_D_COLOR 0xD125
#define LEGEND_E_COLOR 0x9337
#define LEGEND_F_COLOR 0x8AA9
#define LEGEND_G_COLOR 0xE3B8
#define LEGEND_H_COLOR 0x7BEF
#define LEGEND_I_COLOR 0xBDE4
#define LEGEND_J_COLOR 0x15F9 
        }
      }
  }
}

void loop()
{
}
