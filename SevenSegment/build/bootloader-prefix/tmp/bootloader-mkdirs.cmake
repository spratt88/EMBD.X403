# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/spratt/esp/esp-idf/components/bootloader/subproject"
  "/home/spratt/esp/projects/SevenSegment/build/bootloader"
  "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix"
  "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix/tmp"
  "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix/src/bootloader-stamp"
  "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix/src"
  "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/spratt/esp/projects/SevenSegment/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
