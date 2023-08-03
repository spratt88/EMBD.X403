#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_idf_version.h" // for backward compatibility of esp-timer
#include "esp_sleep.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu_api.h"

#include "ulp_main.h"

#define BSP_I2C_NUM 0
#define BSP_I2C_SDA GPIO_NUM_3
#define BSP_I2C_SCL GPIO_NUM_2

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
        mpu_config();
        //mpu_get_acce(&acce);
        //mpu_get_gyro(&gyro);

        ESP_LOGI(MPU, "MPU Initial Test");
        //printf("accex = %3.2f, accey = %3.2f, accez = %3.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        //printf("gyrox = %3.2f, gyroy = %3.2f, gyroz = %3.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

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