idf_component_register(SRCS "simple_accelerometer.c"
                    INCLUDE_DIRS "."
                    REQUIRES soc mpu6050 ulp riscv esp_timer xtensa)