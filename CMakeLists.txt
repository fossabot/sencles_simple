# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET esp32s3)
set(CCACHE_ENABLE 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_tgi)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "key/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "key/client.key" TEXT)
