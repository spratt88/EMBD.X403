#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_adc/adc_continuous.h"
#include "driver/ledc.h"
#include "dcmotor_control.h"

#include "i2c-lcd.h"

#define PIN_SWITCH1                 GPIO_NUM_15

#define I2C_MASTER_SCL_IO           GPIO_NUM_19             /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_20             /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000                      /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_11     /* 0-3.100V */
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#else
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type2.data)
#endif

#define EXAMPLE_READ_LEN                    256

static TaskHandle_t s_task_handle;
static const char *ADCTAG = "ADC Input";

static const char *I2CTAG = "i2c-driver";

static const char *DCMTAG = "DC";
bool motorOn = false;

static adc_channel_t adc_channel = ADC_CHANNEL_4;
uint32_t adc_val;

QueueHandle_t switchQueue;

/* Rising edge switch interrupt handler
 */
static void IRAM_ATTR gpio_interrupt_handler(void *args){
    int pinNumber = (int)args;

    xQueueSendFromISR( switchQueue, &pinNumber, NULL );
}

/**
 * adc config functions
 */
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
        .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        ESP_LOGI(ADCTAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(ADCTAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(ADCTAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

/**
 * i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

unsigned int adc_to_percent( unsigned int val ){
    return( ( val * 100 ) / ( pow( 2, EXAMPLE_ADC_BIT_WIDTH ) ) );
}

void cont_read_adc_task(void *pvParameters)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);
    uint32_t res_av;

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(&adc_channel, sizeof(adc_channel) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    while(1) {

        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

        while (1) {
            ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                res_av = 0;
                //ESP_LOGI("ADC TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (void*)&result[i];
                    uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
                    uint32_t data = EXAMPLE_ADC_GET_DATA(p);
                    /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                    if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
                        //ESP_LOGI(ADCTAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, unit, chan_num, data);
                    } else {
                        ESP_LOGW(ADCTAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", unit, chan_num, data);
                    }
                    if ( i >= 1 ) 
                        res_av = (res_av + data) / 2; 
                }
                /**
                 * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
                 * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
                 * usually you don't need this delay (as this task will block for a while).
                 */
                adc_val = res_av;
                vTaskDelay(1);
            } else if (ret == ESP_ERR_TIMEOUT) {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}

void update_LCD(void *pvParameters) 
{

    int pinNumber;
    while(1) {
        /* Interrupt recieved - update display */
        if (xQueueReceive(switchQueue, &pinNumber, portMAX_DELAY)) {
            // disable the interrupt
            gpio_isr_handler_remove(pinNumber);

            // wait some time while we check for the button to be released
            do
            {
                vTaskDelay(30 / portTICK_PERIOD_MS);
            } while (gpio_get_level(pinNumber) == 0);

            motorOn = !motorOn;
            ESP_LOGI("DCMOTOR", "Set motor state: %d", motorOn);
            
            /* re-enable the interrupt */
            gpio_isr_handler_add(pinNumber, gpio_interrupt_handler, (void *)pinNumber);
        }    

        vTaskDelay(1);
    }
}

void run_motor(void *pvParameters) 
{
    const char *runStatus = "Running";
    const char *stopStatus = "Stopped";
    char buffer[16];
    while (1) {
        /* Write to line 1 of LCD */
        lcd_put_cur(0, 0);
        if (motorOn) { /* Update motor speed */
            // Set duty to 50%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty(adc_to_percent(adc_val))));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
            sprintf(buffer, "State=%8s", runStatus);
            ESP_LOGI("DCMOTOR", "Status: %s", runStatus);
        } else { /* Turn off motor */
            ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0)); 
            sprintf(buffer, "State=%8s", stopStatus);
            ESP_LOGI("DCMOTOR", "Status: %s", stopStatus);
        }
        lcd_send_string(buffer);
        /* Write to line 2 of LCD */
        lcd_put_cur(1, 0);
        sprintf(buffer, "DutyCycle=%3d", adc_to_percent(adc_val));
        ESP_LOGI("DCMOTOR", "DC: %3d", adc_to_percent(adc_val));
        lcd_send_string(buffer);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void switch_init(void)
{
    /* Set GPIO switch to input */
    gpio_set_direction(PIN_SWITCH1, GPIO_MODE_INPUT);

    /* Set switch to pull-down */
    gpio_pulldown_en(PIN_SWITCH1);
    gpio_pullup_dis(PIN_SWITCH1);

    /* Set ISR for rising edge */
    gpio_set_intr_type(PIN_SWITCH1, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_SWITCH1, gpio_interrupt_handler, (void *)PIN_SWITCH1);
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(I2CTAG, "I2C initialized successfully");

    lcd_init();
    lcd_clear();

    // Set the LEDC peripheral configuration
    motor_init();
    motor_direction( true );
    ESP_LOGI(DCMTAG, "DC motor initialized successfully");

    switch_init();
    /* Create queue for interrupts */
    switchQueue = xQueueCreate(10, sizeof(int));
    ESP_LOGI("SWITCH", "Switch initialized successfully");

    /* Run a DC motor according to the position of a potentiometer.  Output the speed of the motor
     * and the status of the motor (running/stopped) on the LCD.  The motor status will be toggled
     * according to the rising edge of a switch input.
     */

    /* Continuously read the ADC input */
    xTaskCreate(cont_read_adc_task, "Read ADC Task", 4096, NULL, 3, NULL);
    
    /* Write motor speed to LCD */
    xTaskCreate(update_LCD, "Update LCD Task", 4096, NULL, 2, NULL);

    /* Run motor */
    xTaskCreate(run_motor, "Run Motor Task", 4096, NULL, 1, NULL);

}