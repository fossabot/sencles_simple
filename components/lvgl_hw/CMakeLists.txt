idf_component_register(SRCS "gc9a01_main_task.c" "gc9a01.c" 
                       INCLUDE_DIRS "include"
                       REQUIRES esp_lcd lvgl lvgl_app esp_timer driver)
