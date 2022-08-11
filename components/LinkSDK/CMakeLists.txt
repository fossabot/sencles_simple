set(include_dirs core core/sysdep core/utils components/data_model components/ota link_main)
file(GLOB c_sdk_srcs
    "link_main/*.c"
    "components/data_model/*.c"
    "components/ota/*.c"
    "core/*.c"
    "core/utils/*.c"
    "core/sysdep/*.c"
    "portfiles/*.c"
    "external/*.c")
idf_component_register(SRCS ${c_sdk_srcs}
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES mbedtls json sensor_hub)