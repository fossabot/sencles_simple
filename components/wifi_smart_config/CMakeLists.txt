idf_component_register(SRCS "wifi_smart_config_main_task.c"
                    INCLUDE_DIRS include
                    REQUIRES wpa_supplicant nvs_flash aliyun esp_wifi main)
