idf_component_register(SRCS "lvgl_hw_main_task.c" "lvgl_hw_gc9a01.c" "lvgl_app.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_lcd lvgl esp_timer driver main)
