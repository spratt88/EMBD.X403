# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/spratt/EMBD.X403/esp/esp-idf/components/bootloader/subproject"
  "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader"
  "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix"
  "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix/tmp"
  "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix/src/bootloader-stamp"
  "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix/src"
  "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/spratt/EMBD.X403/esp/projects/HelloWorld/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
