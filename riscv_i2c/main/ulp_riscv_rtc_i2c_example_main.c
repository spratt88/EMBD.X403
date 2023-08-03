#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu_api.h"

#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static const char *MPU = "MPU";

/************************************************
 * ULP utility APIs
 ************************************************/
static void init_ulp_program(void);

/************************************************
 * RTC I2C utility APIs
 ************************************************/
static void init_i2c(void);


void mpu_config( void ) {
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

static void init_i2c(void)
{
    /* Configure RTC I2C */
    printf("Initializing RTC I2C ...\n");
    ulp_riscv_i2c_cfg_t i2c_cfg = ULP_RISCV_I2C_DEFAULT_CONFIG();
    esp_err_t ret = ulp_riscv_i2c_master_init(&i2c_cfg);
    if (ret!= ESP_OK) {
        printf("ERROR: Failed to initialize RTC I2C. Aborting...\n");
        abort();
    }
}

static void init_ulp_program(void)
{
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;

    esp_err_t err = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    ESP_ERROR_CHECK(err);

    /* The first argument is the period index, which is not used by the ULP-RISC-V timer
     * The second argument is the period in microseconds, which gives a wakeup time period of: 40ms
     */
    ulp_set_wakeup_period(0, 40000);

    /* Start the program */
    err = ulp_riscv_run();
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;

    memset(&acce, 0, sizeof(mpu6050_acce_value_t));
    memset(&gyro, 0, sizeof(mpu6050_gyro_value_t));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    /* Not a wakeup from ULP
     * Initialize RTC I2C
     * Setup BMP180 sensor
     * Store current temperature and pressure values
     * Load the ULP firmware
     * Go to deep sleep
     */
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not a ULP-RISC V wakeup (cause = %d)\n", cause);

        /* Initialize RTC I2C */
        init_i2c();
        ulp_riscv_i2c_master_set_slave_addr( MPU6050_I2C_ADDRESS );

        // TODO: enter vtask delay for 200ms

        uint8_t data = 0;
        ulp_riscv_i2c_master_read_from_device(&data, 1);
        printf("whoami read %d - first try might fail\n", data);

        ulp_riscv_i2c_master_read_from_device(&data, 1);
        printf("whoami read %d - second try\n", data);

        //mpu_config();

        printf("Getting accelerometer vals ...\n");
        mpu_get_acce(&acce);
        printf("Getting gyro vals ...\n");
        mpu_get_gyro(&gyro);

        ESP_LOGI(MPU, "MPU Initial Test");
        printf("accex = %3.2f, accey = %3.2f, accez = %3.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        printf("gyrox = %3.2f, gyroy = %3.2f, gyroz = %3.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

        /* Add a delay for everything to the printed before heading in to deep sleep */
        vTaskDelay(500);

        /* Load ULP firmware
         *
         * The ULP is responsible of monitoring the temperature and pressure values
         * periodically. It will wakeup the main CPU if the temperature and pressure
         * values are above a certain threshold.
         */
        init_ulp_program();
    }

    /* ULP RISC-V read and detected a temperature or pressure above the limit */
    if (cause == ESP_SLEEP_WAKEUP_ULP) {
        printf("ULP RISC-V woke up the main CPU\n");

        /* Pause ULP while we are using the RTC I2C from the main CPU */
        ulp_timer_stop();
        ulp_riscv_halt();

        printf("accex = %3.2f, accey = %3.2f, accez = %3.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        printf("gyrox = %3.2f, gyroy = %3.2f, gyroz = %3.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

        /* Resume ULP and go to deep sleep again */
        ulp_timer_resume();
    }


    /* Add a delay for everything to the printed before heading in to deep sleep */
    vTaskDelay(100);

    /* Go back to sleep, only the ULP RISC-V will run */
    printf("Entering deep sleep\n\n");

    /* RTC peripheral power domain needs to be kept on to keep RTC I2C related configs during sleep */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    esp_deep_sleep_start();

}