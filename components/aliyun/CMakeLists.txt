set(include_dirs core core/sysdep core/utils components/data-model components/ota aliyun_main_task)
file(GLOB c_srcs
    "aliyun_main_task/*.c"
    "components/data-model/*.c"
    "components/ota/*.c"
    "core/*.c"
    "core/utils/*.c"
    "core/sysdep/*.c"
    "portfiles/*.c"
    "external/*.c")
idf_component_register(SRCS ${c_srcs}
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES mbedtls json sensor ir_gree_transceiver main)