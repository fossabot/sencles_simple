idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES  nvs_flash  smart_config lvgl  sht3x driver lvgl_app lvgl_hw sensor_hub dtimer led_strip)
