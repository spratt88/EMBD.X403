#include <stdio.h>
#include "mpu_api.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"

void mpu_config () {
    /* Configure MPU6050 */
    uint8_t config_regs[2] = {GYRO_FS_500DPS << 3,  ACCE_FS_4G << 3};
    ulp_riscv_i2c_master_set_slave_reg_addr(MPU6050_GYRO_CONFIG);
    ulp_riscv_i2c_master_write_to_device(config_regs, sizeof(config_regs));

    /* Wake MPU up */
    uint8_t tmp;
    ulp_riscv_i2c_master_set_slave_reg_addr(MPU6050_PWR_MGMT_1);
    ulp_riscv_i2c_master_read_from_device(&tmp, 1);
    tmp &= (~BIT6);
    ulp_riscv_i2c_master_write_to_device(tmp, sizeof(tmp));

    return;
}

void mpu_get_acce(mpu6050_acce_value_t *const acce_value) {
    float acce_sensitivity;
    mpu6050_raw_acce_value_t raw_acce_value;

    // get acceleration sensitivity
    uint8_t acce_fs;
    ulp_riscv_i2c_master_set_slave_reg_addr(MPU6050_ACCEL_CONFIG);
    ulp_riscv_i2c_master_read_from_device(&acce_fs, 1);
    acce_fs = (acce_fs >> 3) & 0x03;
    switch (acce_fs) {
    case ACCE_FS_2G:
        acce_sensitivity = 16384;
        break;

    case ACCE_FS_4G:
        acce_sensitivity = 8192;
        break;

    case ACCE_FS_8G:
        acce_sensitivity = 4096;
        break;

    case ACCE_FS_16G:
        acce_sensitivity = 2048;
        break;

    default:
        acce_sensitivity = 0;
        break;
    }

    // get raw acceleration values
    uint8_t data_rd[6];
    ulp_riscv_i2c_master_set_slave_reg_addr(MPU6050_ACCEL_XOUT_H);
    ulp_riscv_i2c_master_read_from_device(&data_rd, sizeof(data_rd));
    raw_acce_value.raw_acce_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    raw_acce_value.raw_acce_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    raw_acce_value.raw_acce_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

    // set accel values
    if (acce_sensitivity != 0) {
        acce_value->acce_x = raw_acce_value.raw_acce_x / acce_sensitivity;
        acce_value->acce_y = raw_acce_value.raw_acce_y / acce_sensitivity;
        acce_value->acce_z = raw_acce_value.raw_acce_z / acce_sensitivity;
    }

    return;
}

void mpu_get_gyro(mpu6050_gyro_value_t *const gyro_value) {
    float gyro_sensitivity;
    mpu6050_raw_gyro_value_t raw_gyro_value;

    // get gyro sensitivity
    uint8_t gyro_fs;
    ulp_riscv_i2c_master_set_slave_reg_addr(MPU6050_GYRO_CONFIG);
    ulp_riscv_i2c_master_read_from_device(&gyro_fs, 1);
    gyro_fs = (gyro_fs >> 3) & 0x03;
    switch (gyro_fs) {
    case GYRO_FS_250DPS:
        gyro_sensitivity = 131;
        break;

    case GYRO_FS_500DPS:
        gyro_sensitivity = 65.5;
        break;

    case GYRO_FS_1000DPS:
        gyro_sensitivity = 32.8;
        break;

    case GYRO_FS_2000DPS:
        gyro_sensitivity = 16.4;
        break;

    default:
        gyro_sensitivity = 0;
        break;
    }

    // get raw gryo values
    uint8_t data_rd[6];
    ulp_riscv_i2c_master_set_slave_reg_addr(MPU6050_GYRO_XOUT_H);
    ulp_riscv_i2c_master_read_from_device(&data_rd, sizeof(data_rd));
    raw_gyro_value.raw_gyro_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    raw_gyro_value.raw_gyro_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    raw_gyro_value.raw_gyro_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

    // set gyro values
    if (gyro_sensitivity != 0) {
        gyro_value->gyro_x = raw_gyro_value.raw_gyro_x / gyro_sensitivity;
        gyro_value->gyro_y = raw_gyro_value.raw_gyro_y / gyro_sensitivity;
        gyro_value->gyro_z = raw_gyro_value.raw_gyro_z / gyro_sensitivity;
    }
}