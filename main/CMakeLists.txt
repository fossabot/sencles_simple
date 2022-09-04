idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES nvs_flash wifi_smart_config lvgl driver lvgl_app lvgl_hw sensor led_strip LinkSDK ir_gree_transceiver)
