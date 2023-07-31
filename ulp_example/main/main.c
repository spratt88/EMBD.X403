#include <stdio.h>
#include "mpu6050.h"
//#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_idf_version.h" // for backward compatibility of esp-timer

#define BSP_I2C_NUM 0
#define BSP_I2C_SDA GPIO_NUM_3
#define BSP_I2C_SCL GPIO_NUM_2

static const char *MPU = "MPU";
static mpu6050_handle_t mpu6050_dev = NULL;

static mpu6050_acce_value_t acce;
static mpu6050_gyro_value_t gyro;
static complimentary_angle_t complimentary_angle;

void mpu6050_init()
{
    mpu6050_dev = mpu6050_create(BSP_I2C_NUM, MPU6050_I2C_ADDRESS);
    mpu6050_config(mpu6050_dev, ACCE_FS_4G, GYRO_FS_500DPS);
    mpu6050_wake_up(mpu6050_dev);
}

void mpu6050_read(void *pvParameters)
{
    mpu6050_get_acce(mpu6050_dev, &acce);
    mpu6050_get_gyro(mpu6050_dev, &gyro);
    mpu6050_complimentory_filter(mpu6050_dev, &acce, &gyro, &complimentary_angle);
}

esp_err_t bsp_i2c_init(void)
{
    static bool i2c_initialized = false;

    /* I2C was initialized before */
    if (i2c_initialized) {
        return ESP_OK;
    }

    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(BSP_I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0));

    i2c_initialized = true;

    return ESP_OK;
}

void app_main(void)
{
    bsp_i2c_init();
    mpu6050_init();

    // In order to get accurate calculation of complimentary angle we need fast reading (5ms)
    // FreeRTOS resolution is 10ms, so esp_timer is used
    const esp_timer_create_args_t cal_timer_config = {
        .callback = mpu6050_read,
        .arg = NULL,
        .name = "MPU6050 timer",
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
        .skip_unhandled_events = true,
#endif
        .dispatch_method = ESP_TIMER_TASK
    };
    esp_timer_handle_t cal_timer = NULL;
    esp_timer_create(&cal_timer_config, &cal_timer);
    esp_timer_start_periodic(cal_timer, 5000); // 5ms

    while (1) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        printf("accex = %3.2f, accey = %3.2f, accez = %3.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        printf("gyrox = %3.2f, gyroy = %3.2f, gyroz = %3.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);
        printf("roll = %3.2f, pitch = %3.2f", complimentary_angle.roll, complimentary_angle.pitch);
    }
}