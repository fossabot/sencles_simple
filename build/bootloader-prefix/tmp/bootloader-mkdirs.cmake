# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/esp-idf/components/bootloader/subproject"
  "C:/Users/38326/Desktop/esp32_tgi/build/bootloader"
  "C:/Users/38326/Desktop/esp32_tgi/build/bootloader-prefix"
  "C:/Users/38326/Desktop/esp32_tgi/build/bootloader-prefix/tmp"
  "C:/Users/38326/Desktop/esp32_tgi/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/38326/Desktop/esp32_tgi/build/bootloader-prefix/src"
  "C:/Users/38326/Desktop/esp32_tgi/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/38326/Desktop/esp32_tgi/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
