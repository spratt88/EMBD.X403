# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/spratt/esp/esp-idf/components/ulp/cmake"
  "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main"
  "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix"
  "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix/tmp"
  "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix/src/ulp_main-stamp"
  "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix/src"
  "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix/src/ulp_main-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix/src/ulp_main-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/spratt/esp/projects/riscv_i2c/build/esp-idf/main/ulp_main-prefix/src/ulp_main-stamp${cfgdir}") # cfgdir has leading slash
endif()
