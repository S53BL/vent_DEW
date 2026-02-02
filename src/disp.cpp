// disp.cpp - Display implementation for vent_DEW LVGL version
#include "disp.h"
#include <lvgl.h>

// LVGL objects
static lv_obj_t* kop_btn;
static lv_obj_t* kop_label1;
static lv_obj_t* kop_label2;
static lv_obj_t* kop_label3;

// LVGL display buffer and driver
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenHeight * screenWidth / 10];
TFT_eSPI tft = TFT_eSPI();

// LVGL flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

// Button event handler
static void kop_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        // Zoom in animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, kop_btn);
        lv_anim_set_values(&a, 256, 240);
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    } else if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED) {
        // Zoom out animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, kop_btn);
        lv_anim_set_values(&a, 240, 256);
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    }
}

void initDisplay() {
    // Initialize TFT
    tft.init();
    tft.setRotation(2); // Portrait rotated 180 degrees to match demo

    // Initialize LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenHeight * screenWidth / 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Set background color
    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xBDBDBD), 0); // Light gray background
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_border_opa(screen, LV_OPA_TRANSP, 0);

    // Create KOP button
    createKOPButton();
}

void createKOPButton() {
    // Create button style
    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 15);
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_width(&style_btn, 3);
    lv_style_set_border_color(&style_btn, lv_color_white());
    lv_style_set_shadow_width(&style_btn, 10);
    lv_style_set_shadow_color(&style_btn, lv_color_hex(0x333333));
    lv_style_set_shadow_opa(&style_btn, LV_OPA_60);
    lv_style_set_shadow_ofs_x(&style_btn, 4);
    lv_style_set_shadow_ofs_y(&style_btn, 4);
    lv_style_set_shadow_spread(&style_btn, 0);
    lv_style_set_pad_all(&style_btn, 15);

    // Create button - 230x230 centered on 240x240 screen
    kop_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_pos(kop_btn, 5, 5);  // Centered: (240-230)/2 = 5
    lv_obj_set_size(kop_btn, 230, 230);
    lv_obj_add_style(kop_btn, &style_btn, 0);
    lv_obj_set_style_bg_color(kop_btn, lv_color_hex(BTN_KOP_COLOR), 0);
    lv_obj_add_event_cb(kop_btn, kop_btn_event_cb, LV_EVENT_ALL, NULL);

    // KOP label - larger font for bigger button
    kop_label1 = lv_label_create(kop_btn);
    lv_label_set_text(kop_label1, "KOP");
    lv_obj_set_style_text_font(kop_label1, &lv_font_montserrat_28, 0);
    lv_obj_align(kop_label1, LV_ALIGN_TOP_LEFT, 15, 20);

    // Temperature label - larger font
    kop_label2 = lv_label_create(kop_btn);
    lv_label_set_text(kop_label2, "00.0°");
    lv_obj_set_style_text_font(kop_label2, &lv_font_montserrat_20, 0);
    lv_obj_align(kop_label2, LV_ALIGN_TOP_LEFT, 15, 70);

    // Humidity label - larger font
    kop_label3 = lv_label_create(kop_btn);
    lv_label_set_text(kop_label3, "00.0%");
    lv_obj_set_style_text_font(kop_label3, &lv_font_montserrat_20, 0);
    lv_obj_align(kop_label3, LV_ALIGN_TOP_LEFT, 15, 100);
}

void drawKOPButton() {
    // LVGL handles drawing, just update labels if needed
    // This function kept for compatibility
}
