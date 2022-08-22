idf_component_register(SRCS "lvgl_hw_main_task.c" "lvgl_hw_gc9a01.c" 
                       INCLUDE_DIRS "include"
                       REQUIRES esp_lcd lvgl lvgl_app esp_timer driver)
