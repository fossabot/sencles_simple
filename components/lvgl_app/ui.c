// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: SquareLine_Project

#include "ui.h"
#include "lvgl_app.h"
#include "freertos/queue.h"
#include "math.h"

static void data_anim(lv_obj_t * obj, int32_t value);

extern QueueHandle_t xQueueHumi;
extern QueueHandle_t xQueueTemp;
static float temp_rx, humi_rx, temp_body;


///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_Screen1;
lv_obj_t * ui_humiArc;
lv_obj_t * ui_tempArc;
lv_obj_t * ui_humiLabel;
lv_obj_t * ui_tempLabel;
lv_obj_t * ui_tempLabelnum;
lv_obj_t * ui_humiLabelnum;
lv_obj_t * ui_Labeldegree;
lv_obj_t * ui_Labelpercent;
lv_obj_t * ui_btempLabel;
lv_obj_t * ui_btempLabelnum;
lv_obj_t * ui_timeLabel;
lv_obj_t * ui_btempLabeldegree;
lv_obj_t * ui_count;
lv_obj_t * ui_countss;

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=1
    #error "#error LV_COLOR_16_SWAP should be 1 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////

///////////////////// SCREENS ////////////////////
void ui_Screen1_screen_init(void)
{

    // ui_Screen1

    ui_Screen1 = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

    // ui_humiArc

    ui_humiArc = lv_arc_create(ui_Screen1);

    lv_obj_set_width(ui_humiArc, 240);
    lv_obj_set_height(ui_humiArc, 240);

    lv_obj_set_x(ui_humiArc, 0);
    lv_obj_set_y(ui_humiArc, 0);

    lv_obj_set_align(ui_humiArc, LV_ALIGN_CENTER);

    lv_obj_clear_flag(ui_humiArc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                      LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_arc_set_range(ui_humiArc, 0, 100);
    lv_arc_set_value(ui_humiArc, 20);
    lv_arc_set_bg_angles(ui_humiArc, 90, 9);

    lv_obj_set_style_arc_color(ui_humiArc, lv_color_hex(0x00FF68), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_humiArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_humiArc, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_humiArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    // ui_tempArc

    ui_tempArc = lv_arc_create(ui_Screen1);

    lv_obj_set_width(ui_tempArc, 210);
    lv_obj_set_height(ui_tempArc, 210);

    lv_obj_set_x(ui_tempArc, -1);
    lv_obj_set_y(ui_tempArc, 0);

    lv_obj_set_align(ui_tempArc, LV_ALIGN_CENTER);

    lv_obj_clear_flag(ui_tempArc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_arc_set_range(ui_tempArc, -25, 60);
    lv_arc_set_bg_angles(ui_tempArc, 100, 0);

    lv_obj_set_style_arc_color(ui_tempArc, lv_color_hex(0x00D3FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_tempArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_tempArc, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_tempArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    // ui_humiLabel

    ui_humiLabel = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_humiLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_humiLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_humiLabel, 41);
    lv_obj_set_y(ui_humiLabel, -44);

    lv_obj_set_align(ui_humiLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_humiLabel, "humi");

    lv_obj_clear_flag(ui_humiLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_text_font(ui_humiLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_tempLabel

    ui_tempLabel = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_tempLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_tempLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_tempLabel, -35);
    lv_obj_set_y(ui_tempLabel, -45);

    lv_obj_set_align(ui_tempLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_tempLabel, "temp");

    lv_obj_clear_flag(ui_tempLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_text_font(ui_tempLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_tempLabelnum

    ui_tempLabelnum = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_tempLabelnum, 40);
    lv_obj_set_height(ui_tempLabelnum, 50);

    lv_obj_set_x(ui_tempLabelnum, -34);
    lv_obj_set_y(ui_tempLabelnum, 0);

    lv_obj_set_align(ui_tempLabelnum, LV_ALIGN_CENTER);

    lv_label_set_text(ui_tempLabelnum, "20.000");

    lv_obj_clear_flag(ui_tempLabelnum, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_text_font(ui_tempLabelnum, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_humiLabelnum

    ui_humiLabelnum = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_humiLabelnum, 40);
    lv_obj_set_height(ui_humiLabelnum, 50);

    lv_obj_set_x(ui_humiLabelnum, 48);
    lv_obj_set_y(ui_humiLabelnum, 0);

    lv_obj_set_align(ui_humiLabelnum, LV_ALIGN_CENTER);

    lv_label_set_text(ui_humiLabelnum, "50.000");

    lv_obj_set_style_text_font(ui_humiLabelnum, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Labeldegree

    ui_Labeldegree = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_Labeldegree, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Labeldegree, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Labeldegree, 3);
    lv_obj_set_y(ui_Labeldegree, 8);

    lv_obj_set_align(ui_Labeldegree, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Labeldegree, "*C");

    // ui_Labelpercent

    ui_Labelpercent = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_Labelpercent, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Labelpercent, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Labelpercent, 80);
    lv_obj_set_y(ui_Labelpercent, 8);

    lv_obj_set_align(ui_Labelpercent, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Labelpercent, "%");

    // ui_btempLabel

    ui_btempLabel = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_btempLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_btempLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_btempLabel, 0);
    lv_obj_set_y(ui_btempLabel, 31);

    lv_obj_set_align(ui_btempLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_btempLabel, "Feels Like");

    // ui_btempLabelnum

    ui_btempLabelnum = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_btempLabelnum, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_btempLabelnum, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_btempLabelnum, 2);
    lv_obj_set_y(ui_btempLabelnum, 53);

    lv_obj_set_align(ui_btempLabelnum, LV_ALIGN_CENTER);

    lv_label_set_text(ui_btempLabelnum, "25.000");

    lv_obj_set_style_text_font(ui_btempLabelnum, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_timeLabel

    ui_timeLabel = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_timeLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_timeLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_timeLabel, 2);
    lv_obj_set_y(ui_timeLabel, -66);

    lv_obj_set_align(ui_timeLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_timeLabel, "hr:mi:se");

    lv_obj_set_style_text_font(ui_timeLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_btempLabeldegree

    ui_btempLabeldegree = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_btempLabeldegree, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_btempLabeldegree, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_btempLabeldegree, 48);
    lv_obj_set_y(ui_btempLabeldegree, 53);

    lv_obj_set_align(ui_btempLabeldegree, LV_ALIGN_CENTER);

    lv_label_set_text(ui_btempLabeldegree, "*C");

    // ui_count

    ui_count = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_count, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_count, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_count, -5);
    lv_obj_set_y(ui_count, 74);

    lv_obj_set_align(ui_count, LV_ALIGN_CENTER);

    lv_label_set_text(ui_count, "00000000");

    // ui_countss

    ui_countss = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_countss, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_countss, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_countss, 53);
    lv_obj_set_y(ui_countss, 73);

    lv_obj_set_align(ui_countss, LV_ALIGN_CENTER);

    lv_label_set_text(ui_countss, "sec");

}

void ui_init(void)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_Screen1_screen_init();
    lv_disp_load_scr(ui_Screen1);
}






static void data_anim(lv_obj_t * obj, int32_t value) {

    xQueuePeekFromISR(xQueueTemp, &temp_rx);
    xQueuePeekFromISR(xQueueHumi, &humi_rx);
    temp_body = 1.07*temp_rx + 0.2*(humi_rx/100.0)*6.105*exp((17.27*temp_rx)/(237.7+temp_rx)) - 2.7;


    if (temp_rx > 18 && temp_rx <= 26) {
        lv_label_set_text_fmt(label_temp_index, "%f", temp_rx);
        lv_style_set_text_color(&color_style_temp, lv_palette_main(LV_PALETTE_GREEN));
    } else if (temp_rx >= 26) {
        lv_label_set_text_fmt(label_temp_index, "%f", temp_rx);
        lv_style_set_text_color(&color_style_temp, lv_palette_main(LV_PALETTE_RED));
    } else {
        lv_label_set_text_fmt(label_temp_index, "%f", temp_rx);
        lv_style_set_text_color(&color_style_temp, lv_palette_main(LV_PALETTE_BLUE));
    }


    if (humi_rx > 40 && humi_rx <= 60) {
        lv_label_set_text_fmt(label_humi_index, "%f", humi_rx);
        lv_style_set_text_color(&color_style_humi, lv_palette_main(LV_PALETTE_GREEN));
    } else if (humi_rx > 60) {
        lv_label_set_text_fmt(label_humi_index, "%f", humi_rx);
        lv_style_set_text_color(&color_style_humi, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    } else {
        lv_label_set_text_fmt(label_humi_index, "%f", humi_rx);
        lv_style_set_text_color(&color_style_humi, lv_palette_main(LV_PALETTE_RED));
    }
    lv_label_set_text_fmt(label_body_index, "%f", temp_body);
}