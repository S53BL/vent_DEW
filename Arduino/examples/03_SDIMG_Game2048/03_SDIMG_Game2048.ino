#include "TFT_eSPI.h"
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "WS_QMI8658.h"
#include <Wire.h>
#include <vector>
#include "JpegFunc.h"
#include "FS.h"
#include "SD_MMC.h"
#include <JPEGDecoder.h>
#include <Adafruit_NeoPixel.h>
#include "game_2048_gui.h"
#include "element_images.h"
#include "game2048_contorller.h"
#include "game_2048.h"

//WS2812B RGB PIN
#define PIN 15

//Micro SD Card PIN
#define SD_SCLK_PIN 21
#define SD_MOSI_PIN 18
#define SD_MISO_PIN 16

#define SCALE_SIZE 4
lv_obj_t *game_2048_gui = NULL;
lv_obj_t *img[SCALE_SIZE * SCALE_SIZE];
bool flag = 1;
static lv_style_t default_style;
SemaphoreHandle_t lvgl_mutex = xSemaphoreCreateMutex();
#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#define AIO_LVGL_OPERATE_LOCK(CODE)                          \
    if (pdTRUE == xSemaphoreTake(lvgl_mutex, portMAX_DELAY)) \
    {                                                        \
        CODE;                                                \
        xSemaphoreGive(lvgl_mutex);                          \
    }                         \

GAME2048 game;

struct Game2048AppRunData
{
    int Normal = 0;       //Record the direction of movement
    int BornLocation = 0; //Record the position of the new chess piece
    int *pBoard;
    int *moveRecord;
    BaseType_t xReturned_task_one = pdFALSE;
    TaskHandle_t xHandle_task_one = NULL;
    BaseType_t xReturned_task_two = pdFALSE;
    TaskHandle_t xHandle_task_two = NULL;
};

static Game2048AppRunData *run_data = NULL;



static const uint32_t screenWidth = 240;
static const uint32_t screenHeight = 240;


static lv_color_t buf[screenHeight * screenWidth / 10];
static lv_disp_draw_buf_t draw_buf;


char str1[20];
uint32_t cardSize;
TFT_eSPI tft= TFT_eSPI(240,240);
TFT_eSprite sprite = TFT_eSprite(&tft);
static int16_t x, y;
static bool mountSD = false;


int start=1;
String name[150];
String tmp;
int numTabs=0;
int number=1;
File root;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);


void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.startWrite();
    // tft->writePixels(&color_p->full, w * h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();
    // Initiate DMA - blocking only if last DMA is not complete
    // tft->pushImageDMA(area->x1, area->y1, w, h, bitmap, &color_p->full);

    lv_disp_flush_ready(disp);
}

void QMI8658_Loop_A()
{
      printf("ACCELx:  %f\r\n",QMI8658_get_A_fx());
      printf("ACCELy:  %f\r\n",QMI8658_get_A_fy());
      if(QMI8658_get_A_fx()>=3.7 && flag == 0)
      {
        delay(50);
          if(QMI8658_get_A_fx()>=3.7 && flag == 0)
          active = TURN_RIGHT; //TURN_LEFT
        flag = 1;
        
      }
      else if(QMI8658_get_A_fx()<=-2.2 && flag == 0)
      {
        delay(50);
        if(QMI8658_get_A_fx()<=-2.2 && flag == 0)
        active = TURN_LEFT; //TURN_RIGHT
        flag = 1;
      }

      if(QMI8658_get_A_fy()>=2.4 && flag == 0)
      {
        delay(50);
        if(QMI8658_get_A_fy()>=2.4 && flag == 0)
        active = UP;//DOWN
        flag = 1;
      }
      else if(QMI8658_get_A_fy()<=-3.0 && flag == 0)
      {
        delay(50);
        if(QMI8658_get_A_fy()<=-3.0 && flag == 0)
        active = DOWN;//UP
        flag = 1;
      }
}




/*
 * Randomly refresh a 2
 * Returns the refreshed position
 */

int GAME2048::addRandom(void)
{
    int rand;
    while (1)
    {
        rand = random(3200) % 16;
        
        if (this->board[rand / 4][rand % 4] == 0 )
        {
            Serial.println("2");
            Serial.println(rand/4);
            Serial.println(rand%4);
            this->board[rand / 4][rand % 4] = 2;
            break;
        }
    }
    return rand;
}

/*
 *   Record the original position of the movement. If there are numbers, fill in ABCD according to the direction.
 *   direction 1. Up 2. Down 3. Left 4. Right
 *   In order to reduce the amount of code, it has been optimized. If you need to understand the logic, you can directly look at the code on the left.
 *   It should be more concise (lazy)
 */
void GAME2048::recordLocation(int direction)
{
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            //无数字则为空
            if (board[i][j] == 0)//判断游戏盘中的数字是否等于0
            {
                Location[i][j] = "";//临时字符串
            }
            else
            {
                //有数字根据方向填入ABCD
                switch (direction)
                {
                case 1: //上
                case 2: //下
                    switch (i)
                    {
                    case 0:
                        Location[i][j] = "A";
                        break;
                    case 1:
                        Location[i][j] = "B";
                        break;
                    case 2:
                        Location[i][j] = "C";
                        break;
                    case 3:
                        Location[i][j] = "D";
                        break;
                    }
                    break;
                case 3: //左
                case 4: //右
                    switch (j)
                    {
                    case 0:
                        Location[i][j] = "A";
                        break;
                    case 1:
                        Location[i][j] = "B";
                        break;
                    case 2:
                        Location[i][j] = "C";
                        break;
                    case 3:
                        Location[i][j] = "D";
                        break;
                    }
                    break;
                }
            }
        }
    }
}

/*
 *   By analyzing the changes in the Location variable, obtain and record the moving distance and merged location
 *   direction 1. Up 2. Down 3. Left 4. Right
 *   >4 means merge, -8 means move value
 *   <4 is the value of the move directly
 */
void GAME2048::countMoveRecord(int direction)
{

    //清空
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            moveRecord[i][j] = 0;
        }
    }
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            switch (direction)
            {
            case 1:
            case 2:
                //移动检测
                if (Location[i][j].find("A") != -1)
                {
                    moveRecord[0][j] += i;
                }
                if (Location[i][j].find("B") != -1)
                {
                    moveRecord[1][j] += i - 1;
                }
                if (Location[i][j].find("C") != -1)
                {
                    moveRecord[2][j] += i - 2;
                }
                if (Location[i][j].find("D") != -1)
                {
                    moveRecord[3][j] += i - 3;
                }
                break;
            case 3:
            case 4:
                //移动检测
                if (Location[i][j].find("A") != -1)
                {
                    moveRecord[i][0] += j;
                }
                if (Location[i][j].find("B") != -1)
                {
                    moveRecord[i][1] += j - 1;
                }
                if (Location[i][j].find("C") != -1)
                {
                    moveRecord[i][2] += j - 2;
                }
                if (Location[i][j].find("D") != -1)
                {
                    moveRecord[i][3] += j - 3;
                }
                break;
            }
            //合并检测
            if (Location[i][j].length() == 2)
            {
                moveRecord[i][j] += 8;
            }
        }
    }
}

void GAME2048::moveUp(void)
{
    recordLocation(1); //记录位置
    recordBoard();     //记录数值
    //移动两次
    for (int x = 0; x < 2; x++)
    {
        for (int i = 0; i < SCALE_SIZE - 1; i++)
        {
            for (int j = 0; j < SCALE_SIZE; j++)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = board[i + 1][j];
                    board[i + 1][j] = 0;
                    //动画轨迹记录
                    Location[i][j] = Location[i + 1][j];
                    Location[i + 1][j] = "";
                }
            }
        }
    }

    //相加
    for (int i = 0; i < SCALE_SIZE - 1; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            if (board[i][j] == board[i + 1][j])
            {
                board[i][j] *= 2;
                board[i + 1][j] = 0;
                //动画轨迹记录
                Location[i][j].append(Location[i + 1][j]);
                Location[i + 1][j] = "";
            }
        }
    }

    //移动
    for (int i = 0; i < SCALE_SIZE - 1; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            if (board[i][j] == 0)
            {
                board[i][j] = board[i + 1][j];
                board[i + 1][j] = 0;
                //动画轨迹记录
                Location[i][j] = Location[i + 1][j];
                Location[i + 1][j] = "";
            }
        }
    }
    countMoveRecord(1);
}


void GAME2048::moveDown(void)
{
    recordLocation(2); //记录位置
    recordBoard();
    //移动
    for (int x = 0; x < 2; x++)
    {
        for (int i = SCALE_SIZE - 1; i > 0; i--)
        {
            for (int j = SCALE_SIZE - 1; j >= 0; j--)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = board[i - 1][j];
                    board[i - 1][j] = 0;
                    //动画移动轨迹记录
                    Location[i][j] = Location[i - 1][j];
                    Location[i - 1][j] = "";
                }
            }
        }
    }

    //相加
    for (int i = SCALE_SIZE - 1; i > 0; i--)
    {
        for (int j = SCALE_SIZE - 1; j >= 0; j--)
        {
            if (board[i][j] == board[i - 1][j])
            {
                board[i][j] *= 2;
                board[i - 1][j] = 0;
                //动画合并轨迹记录
                Location[i][j].append(Location[i - 1][j]);
                Location[i - 1][j] = "";
            }
        }
    }

    //移动
    for (int i = SCALE_SIZE - 1; i > 0; i--)
    {
        for (int j = SCALE_SIZE - 1; j >= 0; j--)
        {
            if (board[i][j] == 0)
            {
                board[i][j] = board[i - 1][j];
                board[i - 1][j] = 0;
                //动画移动轨迹记录
                Location[i][j] = Location[i - 1][j];
                Location[i - 1][j] = "";
            }
        }
    }
    countMoveRecord(2);
}

void GAME2048::moveLeft(void)
{
    recordLocation(3); //记录位置
    recordBoard();
    //移动
    for (int x = 0; x < 2; x++)
    {
        for (int i = 0; i < SCALE_SIZE; i++)
        {
            for (int j = 0; j < SCALE_SIZE - 1; j++)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = board[i][j + 1];
                    board[i][j + 1] = 0;
                    //动画移动轨迹记录
                    Location[i][j] = Location[i][j + 1];
                    Location[i][j + 1] = "";
                }
            }
        }
    }
    //相加
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE - 1; j++)
        {
            if (board[i][j] == board[i][j + 1])
            {
                board[i][j] *= 2;
                board[i][j + 1] = 0;
                //动画合并轨迹记录
                Location[i][j].append(Location[i][j + 1]);
                Location[i][j + 1] = "";
            }
        }
    }
    //移动
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE - 1; j++)
        {
            if (board[i][j] == 0)
            {
                board[i][j] = board[i][j + 1];
                board[i][j + 1] = 0;
                //动画移动轨迹记录
                Location[i][j] = Location[i][j + 1];
                Location[i][j + 1] = "";
            }
        }
    }
    countMoveRecord(3);
}
void GAME2048::moveRight(void)
{
    recordLocation(4); //记录位置
    recordBoard();
    //移动两次
    for (int x = 0; x < 2; x++)
    {
        for (int i = SCALE_SIZE - 1; i >= 0; i--)
        {
            for (int j = SCALE_SIZE - 1; j > 0; j--)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = board[i][j - 1];
                    board[i][j - 1] = 0;
                    //动画移动轨迹记录
                    Location[i][j] = Location[i][j - 1];
                    Location[i][j - 1] = "";
                }
            }
        }
    }

    //相加
    for (int i = SCALE_SIZE - 1; i >= 0; i--)
    {
        for (int j = SCALE_SIZE - 1; j > 0; j--)
        {
            if (board[i][j] == board[i][j - 1])
            {
                board[i][j] *= 2;
                board[i][j - 1] = 0;
                //动画合并轨迹记录
                Location[i][j].append(Location[i][j - 1]);
                Location[i][j - 1] = "";
            }
        }
    }

    //移动
    for (int i = SCALE_SIZE - 1; i >= 0; i--)
    {
        for (int j = SCALE_SIZE - 1; j > 0; j--)
        {
            if (board[i][j] == 0)
            {
                board[i][j] = board[i][j - 1];
                board[i][j - 1] = 0;
                //动画移动轨迹记录
                Location[i][j] = Location[i][j - 1];
                Location[i][j - 1] = "";
            }
        }
    }
    countMoveRecord(4);
}

/*
 * Judge() determines the current game status
 * Return 0: the game can continue
 * Return 1: Game win
 * Return 2: The game cannot continue, failed
 */
int GAME2048::judge(void)
{
    //判赢
    for (int i = 0; i <= SCALE_SIZE * SCALE_SIZE; i++)
    {
        if (board[i / 4][i % 4] >= WIN_SCORE)
        {
            return 1; // Win
        }
    }
    //判空
    for (int i = 0; i <= SCALE_SIZE * SCALE_SIZE; i++)
    {
        if (board[i / 4][i % 4] == 0)
        {
            return 0;
        }
    }

    //判相邻相同
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            if (i < 3)
            {
                if (board[i][j] == board[i + 1][j])
                {
                    return 0;
                }
            }
            if (j < 3)
            {
                if (board[i][j] == board[i][j + 1])
                {
                    return 0;
                }
            }
        }
    }

    return 2; // Defeatd
}



void game_2048_gui_init(void)
{
    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style, lv_color_hex(0x000000));

    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == game_2048_gui)
        return;
    lv_obj_clean(act_obj); // 清空此前页面

    //创建屏幕对象
    game_2048_gui = lv_obj_create(NULL);
    lv_obj_add_style(game_2048_gui, &default_style, LV_STATE_DEFAULT);

    for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
    {
        img[i] = lv_img_create(game_2048_gui);
        lv_img_set_src(img[i], &N0);
        lv_obj_align(img[i], LV_ALIGN_TOP_LEFT, 8 + i % 4 * 58, 8 + i / 4 * 58);
    }
    lv_scr_load(game_2048_gui);
}

void taskTwo(void *parameter)
{
    while (1)
    {
        // LVGL任务主函数
        AIO_LVGL_OPERATE_LOCK(lv_task_handler();)
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    Serial.println("Ending lv_task_handler");
    vTaskDelete(NULL);
}



void game_2048_init(void)
{
    // 初始化运行时的参数
    game_2048_gui_init();

    // randomSeed(analogRead(25));
    // 初始化运行时参数
    run_data = (Game2048AppRunData *)calloc(1, sizeof(Game2048AppRunData));
    

    run_data->xReturned_task_two = xTaskCreate(
        taskTwo,                      /*任务函数*/
        "TaskTwo",                    /*带任务名称的字符串*/
        8 * 1024,                     /*堆栈大小，单位为字节*/
        NULL,                         /*作为任务输入传递的参数*/
        1,                            /*任务的优先级*/
        &run_data->xHandle_task_two);

    game.init();
    run_data->pBoard = game.getBoard();
    run_data->moveRecord = game.getMoveRecord();

    int new1 = game.addRandom();
    int new2 = game.addRandom();
    AIO_LVGL_OPERATE_LOCK(showBoard(run_data->pBoard);)
    //棋子出生动画
    AIO_LVGL_OPERATE_LOCK(born(new1);)
    AIO_LVGL_OPERATE_LOCK(born(new2);)

    

    delay(300);
    // 防止进入游戏时，误触发了向上
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void game_2048_process(const ACTIVE_TYPE active)
{
    // if (RETURN == active)
    // {
    //     sys->app_exit(); // 退出APP
    //     return;
    // }

    // 具体操作TURN_LEFT  TURN_RIGHT 
    if (active == TURN_RIGHT && flag == 1)
    {
        game.moveRight();
        if (game.comparePre() == 0)
        {
            AIO_LVGL_OPERATE_LOCK(showAnim(run_data->moveRecord, 4);)
            delay(700);
            AIO_LVGL_OPERATE_LOCK(showNewBorn(game.addRandom(), run_data->pBoard);)
        }
        flag = 0;
    }
    else if (active == TURN_LEFT  && flag == 1)  //TURN_LEFT
    {
        game.moveLeft();
        if (game.comparePre() == 0)
        {
            AIO_LVGL_OPERATE_LOCK(showAnim(run_data->moveRecord, 3);)
            delay(700);
            AIO_LVGL_OPERATE_LOCK(showNewBorn(game.addRandom(), run_data->pBoard);)
        }
        flag = 0;
    }
    else if (active == UP  && flag == 1) 
    {
        game.moveUp();
        if (game.comparePre() == 0)
        {
            AIO_LVGL_OPERATE_LOCK(showAnim(run_data->moveRecord, 1);)
            delay(700);
            AIO_LVGL_OPERATE_LOCK(showNewBorn(game.addRandom(), run_data->pBoard);)
        }
        flag = 0;
    }  
    else if ( active== DOWN && flag == 1) 
    {
        game.moveDown();
        if (game.comparePre() == 0)
        {
            AIO_LVGL_OPERATE_LOCK(showAnim(run_data->moveRecord, 2);)
            delay(700);
            AIO_LVGL_OPERATE_LOCK(showNewBorn(game.addRandom(), run_data->pBoard);)
        }
        flag = 0;
    }

    if (game.judge() == 1)
    {
        //   rgb.setRGB(0, 255, 0);
        Serial.println("you win!");
    }
    else if (game.judge() == 2)
    {
        //   rgb.setRGB(255, 0, 0);
        Serial.println("you lose!");
    }

    // If the program needs it, you can add a delay
    delay(300);
}


/*
 * Please add other functions as needed
 */
void display_game_2048(const char *file_name, lv_scr_load_anim_t anim_type)
{
}

void game_2048_gui_del(void)
{
    if (NULL != game_2048_gui)
    {
        lv_obj_clean(game_2048_gui);
        game_2048_gui = NULL;
    }

    // 手动清除样式，防止内存泄漏
    // lv_style_reset(&default_style);
}


//lv_anim_exec_xcb_t animation parameters for increasing width and height at the same time
static void anim_size_cb(lv_obj_t *var, int32_t v)
{
    lv_obj_set_size(var, v, v);
}

//lv_anim_exec_xcb_t animation parameters for diagonal movement
static void anim_pos_cb(lv_obj_t *var, int32_t v)
{
    lv_obj_set_pos(var, v, v);
}


/*
 * Birth Animation
 * i: birth location
 */
void born(int i)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_size_cb);
    lv_anim_set_var(&a, img[i]);
    lv_anim_set_time(&a, 500);

    
    /* 在动画中设置路径 */
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_values(&a, 0, 50);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_values(&a, lv_obj_get_x(img[i]) + 25, lv_obj_get_x(img[i]));
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(img[i]) + 25, lv_obj_get_y(img[i]));
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_x);
    lv_anim_set_values(&a, -25, 0);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_y);
    lv_anim_set_values(&a, -25, 0);
    lv_anim_start(&a);
}
/*
 * Merge Animation
 * i: merge position
 */
void zoom(int i)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_size_cb);
    lv_anim_set_var(&a, img[i]);
    lv_anim_set_delay(&a, 400);
    lv_anim_set_time(&a, 100);
    //播完后回放
    lv_anim_set_playback_delay(&a, 0);
    lv_anim_set_playback_time(&a, 100);

    //线性动画
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_values(&a, 50, 56);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_values(&a, lv_obj_get_x(img[i]), lv_obj_get_x(img[i]) - 3);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(img[i]), lv_obj_get_y(img[i]) - 3);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_x);
    lv_anim_set_values(&a, 0, 3);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_y);
    lv_anim_set_values(&a, 0, 3);
    lv_anim_start(&a);
}

/*
 * Mobile Animation
 * i: target object to move
 * direction: the direction of movement, lv_obj_set_x or lv_obj_set_y
 * dist: moving distance, such as 1, -1
 */
void move(int i, lv_anim_exec_xcb_t direction, int dist)
{
    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)direction);
    lv_anim_set_var(&a, img[i]);
    lv_anim_set_time(&a, 500);
    if (direction == (lv_anim_exec_xcb_t)lv_obj_set_x)
    {
        lv_anim_set_values(&a, lv_obj_get_x(img[i]), lv_obj_get_x(img[i]) + dist * 58);
    }
    else
    {
        lv_anim_set_values(&a, lv_obj_get_y(img[i]), lv_obj_get_y(img[i]) + dist * 58);
    }

    // 在动画中设置路径
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);

    lv_anim_start(&a);
}

//Get the image content object
const lv_img_dsc_t *getN(int i)
{
    switch (i)
    {
    case 0:
        return &N0;
    case 2:
        return &N2;
    case 4:
        return &N4;
    case 8:
        return &N8;
    case 16:
        return &N16;
    case 32:
        return &N32;
    case 64:
        return &N64;
    case 128:
        return &N128;
    case 256:
        return &N256;
    case 512:
        return &N512;
    case 1024:
        return &N1024;
    case 2048:
        return &N2048;
    default:
        return &N0;
    }
}

//Refresh chessboard LV_ALIGN_TOP_LEFT
void showBoard(int *map)
{
    for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
    {
        lv_img_set_src(img[i], getN(map[i]));
        lv_obj_align(img[i], LV_ALIGN_TOP_LEFT, 8 + i % 4 * 58, 8 + i / 4 * 58);
    }
}

/*
 * showAnim————Update the board with animation
 * animMap: Animation track array
 * direction: the direction of movement, 1. up 2. down 3. left 4. right
 */
void showAnim(int *animMap, int direction)
{
    lv_anim_exec_xcb_t Normal;
    switch (direction)
    {
    case 1:
    case 2:
        Normal = (lv_anim_exec_xcb_t)lv_obj_set_y;
        break;
    case 3:
    case 4:
        Normal = (lv_anim_exec_xcb_t)lv_obj_set_x;
        break;
    }

    //移动和合并
    for (int i = 0; i < 16; i++)
    {
        if (animMap[i] > 4)
        {
            zoom(i);
            move(i, Normal, animMap[i] - 8);
        }
        else if (animMap[i] != 0)
        {
            move(i, Normal, animMap[i]);
        }
    }
}

/*
 * newborn: the position of the new chess piece
 * map: the address of the new chessboard
 */
void showNewBorn(int newborn, int *map)
{
    //展示新棋盘
    showBoard(map);
    //出现
    // born(newborn);
}


static int jpegDrawCallback(JPEGDRAW *pDraw)
{
    tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight,pDraw->pPixels);
    return 1;
}



void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
 root = fs.open("/","r");
 root.rewindDirectory();
      
  while (true)
  {
    File entry =  root.openNextFile();
    if (! entry)
    {break;}
   
    tmp=entry.name();
    name[numTabs]="/"+tmp;
    Serial.println(entry.name());
    entry.close();
    numTabs++;
  }

}




void setup() {

  Serial.begin(115200);
  tft.begin();
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenHeight * screenWidth / 10);
  // /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  // /*Change the following line to your display resolution*  *更改下面一行为您的显示分辨率*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  QMI8658_Init();
  
  // tft.invertDisplay(1);

  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
  pinMode(PIN, OUTPUT);  
  //===========================================================
  SD_MMC.setPins(SD_SCLK_PIN, SD_MOSI_PIN,SD_MISO_PIN);
  if (!SD_MMC.begin("/sdcard", true, true,4000000)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
  //===========================================================
  listDir(SD_MMC, "/", 0); // Change the root directory as needed

}

bool AT = 1;
void loop() {

  if(AT == 1){
    tft.fillScreen(TFT_RED);
    digitalWrite(PIN, HIGH);
    // Some example procedures showing how to display to the pixels:
    colorWipe(strip.Color(255, 0, 0), 50); // Red
    tft.fillScreen(TFT_GREEN);
    colorWipe(strip.Color(0, 255, 0), 50); // Green
    tft.fillScreen(TFT_BLUE);
    colorWipe(strip.Color(0, 0, 255), 50); // Blue
    colorWipe(strip.Color(0, 0, 0), 10);
    AT=0;
    for(number=0;number<numTabs;number++){
      jpegDraw(name[number].c_str(), jpegDrawCallback, true, 0, 0, 240, 240);
      delay(500);
    }
    
    game_2048_init();
  }
  lv_timer_handler();
  delay(1);
  QMI8658_Loop_A();
  game_2048_process(active);
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}