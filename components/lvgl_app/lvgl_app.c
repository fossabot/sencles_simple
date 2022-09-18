// Copyright 2022 JeongYeham
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lvgl_app.h"
#include "math.h"
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_type.h"

extern QueueHandle_t xQueueSenData;

///////////////////// VARIABLES ////////////////////
lv_obj_t *ui_Screen1;
lv_obj_t *ui_humiLabel;
lv_obj_t *ui_tempLabel;
lv_obj_t *ui_Labeldegree;
lv_obj_t *ui_Labelpercent;
lv_obj_t *ui_btempLabel;
lv_obj_t *ui_timeLabel;
lv_obj_t *ui_btempLabeldegree;
lv_obj_t *ui_count;
lv_obj_t *ui_countss;

typedef struct _lv_refresh_detail
{
    struct _lv_clock_detail
    {
        lv_obj_t *time_label;
        lv_obj_t *date_label;
        lv_obj_t *weekday_label;
    } lv_clock;
    struct _lv_humiture_detail
    {
        lv_obj_t *ui_humiArc;
        lv_obj_t *ui_tempArc;
        lv_obj_t *ui_tempLabelnum;
        lv_obj_t *ui_humiLabelnum;
        lv_obj_t *ui_btempLabelnum;
    } lv_humiture;

} lv_refresh_t;

static lv_refresh_t lv_refresh = {0};

static void refresh_task_callback(lv_timer_t *timer)
{

    static const char *week_day[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static time_t current_time;
    static time_t examtime = 1671843600;
    static struct tm *time_info;

    current_time = time(NULL);

    time_info = localtime(&current_time);

    int year = time_info->tm_year + 1900;
    int month = time_info->tm_mon + 1;
    int day = time_info->tm_mday;
    int weekday = time_info->tm_wday;
    int hour = time_info->tm_hour;
    int minutes = time_info->tm_min;
    int second = time_info->tm_sec;

    static sensor_data_t lvgl_recv_data ;
    xQueuePeekFromISR(xQueueSenData, &lvgl_recv_data);

    if (timer != NULL && timer->user_data != NULL)
    {
        lv_refresh_t *refresh = (lv_refresh_t *)(timer->user_data);
        lv_label_set_text_fmt(refresh->lv_clock.time_label, "%02d:%02d:%02d", hour, minutes, second);
        lv_label_set_text_fmt(refresh->lv_clock.date_label, "%d-%02d-%02d", year, month, day);
        lv_label_set_text_fmt(refresh->lv_clock.weekday_label, "%s", week_day[weekday]);
        lv_label_set_text_fmt(ui_count, "%ld", (uint32_t)difftime(examtime, current_time));

        lv_label_set_text_fmt(refresh->lv_humiture.ui_tempLabelnum, "%0.3f", lvgl_recv_data.humiture.temperature);
        lv_label_set_text_fmt(refresh->lv_humiture.ui_humiLabelnum, "%0.3f", lvgl_recv_data.humiture.humidity);
        lv_label_set_text_fmt(refresh->lv_humiture.ui_btempLabelnum, "%0.3f", lvgl_recv_data.humiture.body_temperature); 
        lv_arc_set_value(refresh->lv_humiture.ui_tempArc, (int)lvgl_recv_data.humiture.temperature);
        lv_arc_set_value(refresh->lv_humiture.ui_humiArc, (int)lvgl_recv_data.humiture.humidity);

    }
}

void ui_Screen1_screen_init(void)
{

    // ui_Screen1

    ui_Screen1 = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

    // ui_humiArc

    lv_refresh.lv_humiture.ui_humiArc = lv_arc_create(ui_Screen1);

    lv_obj_set_width(lv_refresh.lv_humiture.ui_humiArc, 240);
    lv_obj_set_height(lv_refresh.lv_humiture.ui_humiArc, 240);

    lv_obj_set_x(lv_refresh.lv_humiture.ui_humiArc, 0);
    lv_obj_set_y(lv_refresh.lv_humiture.ui_humiArc, 0);

    lv_obj_set_align(lv_refresh.lv_humiture.ui_humiArc, LV_ALIGN_CENTER);

    lv_obj_clear_flag(lv_refresh.lv_humiture.ui_humiArc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                                             LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_arc_set_range(lv_refresh.lv_humiture.ui_humiArc, 0, 100);
    lv_arc_set_value(lv_refresh.lv_humiture.ui_humiArc, 10);
    lv_arc_set_bg_angles(lv_refresh.lv_humiture.ui_humiArc, 270, 90);

    lv_obj_set_style_shadow_spread(lv_refresh.lv_humiture.ui_humiArc, 120, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    // lv_obj_set_style_arc_color(lv_refresh.lv_humiture.ui_humiArc, lv_color_hex(0x00FF68), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(lv_refresh.lv_humiture.ui_humiArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(lv_refresh.lv_humiture.ui_humiArc, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lv_refresh.lv_humiture.ui_humiArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(lv_refresh.lv_humiture.ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(lv_refresh.lv_humiture.ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(lv_refresh.lv_humiture.ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(lv_refresh.lv_humiture.ui_humiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    // ui_tempArc

    lv_refresh.lv_humiture.ui_tempArc = lv_arc_create(ui_Screen1);

    lv_obj_set_width(lv_refresh.lv_humiture.ui_tempArc, 240);
    lv_obj_set_height(lv_refresh.lv_humiture.ui_tempArc, 240);

    lv_obj_set_x(lv_refresh.lv_humiture.ui_tempArc, 0);
    lv_obj_set_y(lv_refresh.lv_humiture.ui_tempArc, 0);

    lv_obj_set_align(lv_refresh.lv_humiture.ui_tempArc, LV_ALIGN_CENTER);

    lv_obj_clear_flag(lv_refresh.lv_humiture.ui_tempArc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                      LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_arc_set_range(lv_refresh.lv_humiture.ui_tempArc, -25, 60);
    lv_arc_set_value(lv_refresh.lv_humiture.ui_tempArc, -20);
    lv_arc_set_bg_angles(lv_refresh.lv_humiture.ui_tempArc, 90, 270);

    lv_obj_set_style_arc_color(lv_refresh.lv_humiture.ui_tempArc, lv_color_hex(0x00D3FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(lv_refresh.lv_humiture.ui_tempArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(lv_refresh.lv_humiture.ui_tempArc, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lv_refresh.lv_humiture.ui_tempArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(lv_refresh.lv_humiture.ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(lv_refresh.lv_humiture.ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(lv_refresh.lv_humiture.ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(lv_refresh.lv_humiture.ui_tempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    // ui_humiLabel

    ui_humiLabel = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_humiLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_humiLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_humiLabel, 41);
    lv_obj_set_y(ui_humiLabel, -45);

    lv_obj_set_align(ui_humiLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_humiLabel, "humi");

    lv_obj_clear_flag(ui_humiLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                                        LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_text_font(ui_humiLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_tempLabel

    ui_tempLabel = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_tempLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_tempLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_tempLabel, -40);
    lv_obj_set_y(ui_tempLabel, -45);

    lv_obj_set_align(ui_tempLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_tempLabel, "temp");

    lv_obj_clear_flag(ui_tempLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                                        LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_text_font(ui_tempLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_tempLabelnum

    lv_refresh.lv_humiture.ui_tempLabelnum = lv_label_create(ui_Screen1);

    lv_obj_set_width(lv_refresh.lv_humiture.ui_tempLabelnum, 40);
    lv_obj_set_height(lv_refresh.lv_humiture.ui_tempLabelnum, 50);

    lv_obj_set_x(lv_refresh.lv_humiture.ui_tempLabelnum, -39);
    lv_obj_set_y(lv_refresh.lv_humiture.ui_tempLabelnum, 0);

    lv_obj_set_align(lv_refresh.lv_humiture.ui_tempLabelnum, LV_ALIGN_CENTER);

    lv_label_set_text(lv_refresh.lv_humiture.ui_tempLabelnum, "20.000");

    lv_obj_clear_flag(lv_refresh.lv_humiture.ui_tempLabelnum, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                                           LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_text_font(lv_refresh.lv_humiture.ui_tempLabelnum, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_humiLabelnum

    lv_refresh.lv_humiture.ui_humiLabelnum = lv_label_create(ui_Screen1);

    lv_obj_set_width(lv_refresh.lv_humiture.ui_humiLabelnum, 40);
    lv_obj_set_height(lv_refresh.lv_humiture.ui_humiLabelnum, 50);

    lv_obj_set_x(lv_refresh.lv_humiture.ui_humiLabelnum, 43);
    lv_obj_set_y(lv_refresh.lv_humiture.ui_humiLabelnum, 0);

    lv_obj_set_align(lv_refresh.lv_humiture.ui_humiLabelnum, LV_ALIGN_CENTER);

    lv_label_set_text(lv_refresh.lv_humiture.ui_humiLabelnum, "50.000");

    lv_obj_set_style_text_font(lv_refresh.lv_humiture.ui_humiLabelnum, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Labeldegree

    ui_Labeldegree = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_Labeldegree, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Labeldegree, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Labeldegree, -2);
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
    lv_obj_set_y(ui_btempLabel, 30);

    lv_obj_set_align(ui_btempLabel, LV_ALIGN_CENTER);

    lv_label_set_text(ui_btempLabel, "Feels Like");

    // ui_btempLabelnum

    lv_refresh.lv_humiture.ui_btempLabelnum = lv_label_create(ui_Screen1);

    lv_obj_set_width(lv_refresh.lv_humiture.ui_btempLabelnum, LV_SIZE_CONTENT);
    lv_obj_set_height(lv_refresh.lv_humiture.ui_btempLabelnum, LV_SIZE_CONTENT);

    lv_obj_set_x(lv_refresh.lv_humiture.ui_btempLabelnum, 2);
    lv_obj_set_y(lv_refresh.lv_humiture.ui_btempLabelnum, 53);

    lv_obj_set_align(lv_refresh.lv_humiture.ui_btempLabelnum, LV_ALIGN_CENTER);

    lv_label_set_text(lv_refresh.lv_humiture.ui_btempLabelnum, "25.000");

    lv_obj_set_style_text_font(lv_refresh.lv_humiture.ui_btempLabelnum, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_btempLabeldegree

    ui_btempLabeldegree = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_btempLabeldegree, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_btempLabeldegree, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_btempLabeldegree, 48);
    lv_obj_set_y(ui_btempLabeldegree, 53);

    lv_obj_set_align(ui_btempLabeldegree, LV_ALIGN_CENTER);

    lv_label_set_text(ui_btempLabeldegree, "*C");

    // ui_timeLabel

    /*Time display*/
    lv_refresh.lv_clock.time_label = lv_label_create(ui_Screen1);
    lv_obj_set_width(lv_refresh.lv_clock.time_label, 100);
    lv_obj_set_height(lv_refresh.lv_clock.time_label, 20);

    lv_obj_set_x(lv_refresh.lv_clock.time_label, 3);
    lv_obj_set_y(lv_refresh.lv_clock.time_label, -62);

    lv_obj_set_align(lv_refresh.lv_clock.time_label, LV_ALIGN_CENTER);

    /*Date display*/
    lv_refresh.lv_clock.date_label = lv_label_create(ui_Screen1);
    lv_obj_set_width(lv_refresh.lv_clock.date_label, 100);
    lv_obj_set_height(lv_refresh.lv_clock.date_label, 20);

    lv_obj_set_x(lv_refresh.lv_clock.date_label, 10);
    lv_obj_set_y(lv_refresh.lv_clock.date_label, -80);

    lv_obj_set_align(lv_refresh.lv_clock.date_label, LV_ALIGN_CENTER);

    /*Week display*/
    lv_refresh.lv_clock.weekday_label = lv_label_create(ui_Screen1);
    lv_obj_set_width(lv_refresh.lv_clock.weekday_label, 100);
    lv_obj_set_height(lv_refresh.lv_clock.weekday_label, 20);

    lv_obj_set_x(lv_refresh.lv_clock.weekday_label, 70);
    lv_obj_set_y(lv_refresh.lv_clock.weekday_label, -62);

    lv_obj_set_align(lv_refresh.lv_clock.weekday_label, LV_ALIGN_CENTER);

    // ui_count

    ui_count = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_count, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_count, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_count, -7);
    lv_obj_set_y(ui_count, 74);

    lv_obj_set_align(ui_count, LV_ALIGN_CENTER);

    lv_label_set_text(ui_count, "00000000");

    // ui_countss

    ui_countss = lv_label_create(ui_Screen1);

    lv_obj_set_width(ui_countss, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_countss, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_countss, 50);
    lv_obj_set_y(ui_countss, 73);

    lv_obj_set_align(ui_countss, LV_ALIGN_CENTER);

    lv_label_set_text(ui_countss, "sec");
}

void ui_init(void)
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              false, LV_FONT_DEFAULT);
    lv_timer_t *task_timer = lv_timer_create(refresh_task_callback, 1000, (void *)&lv_refresh);

    if (task_timer != NULL)
    {
        lv_disp_set_theme(dispp, theme);
        ui_Screen1_screen_init();
        lv_disp_load_scr(ui_Screen1);
    }
}
