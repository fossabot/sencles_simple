idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES nvs_flash wifi_smart_config lvgl driver lvgl_hw sensor lightbulb aliyun ir_gree_transceiver)
                        