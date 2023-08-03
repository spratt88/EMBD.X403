#include <stdint.h>
#include <stdlib.h>
#include "ulp_riscv.h"
#include "ulp_riscv_utils.h"
#include "ulp_riscv_i2c_ulp_core.h"
#include "../mpu_api.h"

/************************************************
 * Shared data between main CPU and ULP
 ************************************************/
mpu6050_acce_value_t acce;
mpu6050_gyro_value_t gyro;
int16_t accex_threshold = 1;
int16_t gyrox_threshold = 100;

int main() {
    /* Configure MPU */
    mpu_config();


    while (1) {
        /* Read MPU accelerometer */
        mpu_get_acce(&acce);

        /* Read MPU gyro */
        mpu_get_gyro(&gyro);

        /* Wakeup the main CPU if either the accelerometer or gyro values
         * are more than their respective threshold values.
         */
        if ((acce.acce_x > accex_threshold) || (gyro.gyro_x > gyrox_threshold)) {
            ulp_riscv_wakeup_main_processor();
        }
    }

    return 0;
}

