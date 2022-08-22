idf_component_register(SRCS "led_strip_main_task.c" "led_strip_encoder.c"
                       INCLUDE_DIRS include
                       REQUIRES main sensor_hub)
