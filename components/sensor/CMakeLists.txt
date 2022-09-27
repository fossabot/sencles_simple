set(c_srcs
    "sensor_hub/hal/humiture_hal.c"
    "sensor_hub/hal/imu_hal.c"
    "sensor_hub/hal/light_sensor_hal.c"
    "sensor_hub/iot_sensor_hub.c"
    "sensor_hub/sensor_hub_main_task.c"
    "sensor_hub/sensors_event.c"
    #////////////////////////////////
    "sht3x/sht3x.c"
    "sht4x/sht4x.c"
    "veml7700/veml7700.c")

idf_component_register(SRCS "${c_srcs}"
                        INCLUDE_DIRS "sensor_hub/include" "sht3x/include" "sht4x/include" "veml7700/include"
                        REQUIRES esp_event esp_timer bus main)