//#include <stdio.h>
//#include <stdlib.h>
//#include "esp_log.h"
//#include "sdkconfig.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define USER_SWITCH_PIN 14

#define UART_NUM UART_NUM_1
#define UART_RX_PIN GPIO_NUM_4
#define UART_TX_PIN GPIO_NUM_5
#define LED_PIN GPIO_NUM_19

// Task handles
TaskHandle_t task1Handle, task2Handle, task3Handle, task4Handle;

// Queue handle for user switch interrupt
QueueHandle_t switchQueue;

// LED state
bool ledState = false;

// Task function for Task 1
void task1(void *pvParameters) {
    while (1) {
        // Print message indicating Task 1 is running
        printf("Tsk1-P1 <-\n");

        // Run for about 100 times with a task delay of 1
        for (int i = 0; i < 100; i++) {
            // Delay for 1 tick
            vTaskDelay(1);
        }

        // Print message indicating Task 1 is about to block
        printf("Tsk1-P1 ->\n");

        // Block for 10 milliseconds
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Task function for Task 2
void task2(void *pvParameters) {
    while (1) {
        // Print message indicating Task 2 is running
        printf("\tTsk2-P2 <-\n");

        // Run for about 10 times with a task delay of 1
        for (int i = 0; i < 10; i++) {
            // Delay for 1 tick
            vTaskDelay(1);
        }

        // Print message indicating Task 2 is about to block
        printf("\tTsk2-P2 ->\n");

        // Block for 250 milliseconds
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// Task function for Task 3
void task3(void *pvParameters) {
    uint8_t data[1024];
    int length = 0;
    int ledOn = 0;
    
    // Wait until UART reads a character
    //while (uart_read_bytes(UART_NUM, &data, 1, portMAX_DELAY) <= 0) {
    while (1) {
        //uart_get_buffered_data_len(UART_NUM, (size_t*)&length);
        length = uart_read_bytes(UART_NUM, data, 10, pdMS_TO_TICKS(100));

        if (length > 0) {
            //uart_flush(UART_NUM);

            // Print message indicating Task 3 is running
            printf("\t\tTsk3-P3 <-\n");

            // Process the received character
            printf("\t\tReceived character: %c\n", data[0]);

            // Toggle LED if 'L' or 'l' is pressed
            if (data[0] == 'L' || data[0] == 'l') {
                ledOn = 1;  
            } else {
                ledOn = 0;
            }
           gpio_set_level(LED_PIN, ledOn);

            // Run for about 50 times with a task delay of 1
            for (int i = 0; i < 50; i++) {
                // Delay for 1 tick
                vTaskDelay(1);
            }

            // Print message indicating Task 3 is about to block
            printf("\t\tTsk3-P3 ->\n");
        }
    }
}

// Task function for Task 4
void task4(void *pvParameters) {
    while (1) {
        // Wait for user switch message from the queue
        int switchPin;
        if (xQueueReceive(switchQueue, &switchPin, portMAX_DELAY) == pdTRUE) {
            // Check if the switch pin is the user switch
            if (switchPin == USER_SWITCH_PIN) {
                // Print message indicating Task 4 is running
                printf("\t\t\tTsk4-P4 <-\n");

                // Run for about 10 times with a task delay of 1
                for (int i = 0; i < 10; i++) {
                    // Delay for 1 tick
                    vTaskDelay(1);
                }

                // Print message indicating Task 4 is about to block
                printf("\t\t\tTsk4-P4 ->\n");
            }
        }
    }
}

// User switch interrupt service routine
void IRAM_ATTR user_switch_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Get the pin number that caused the interrupt
    uint32_t switchPin = (uint32_t)arg;

    // Send the switch pin number to the switch queue
    xQueueSendFromISR(switchQueue, &switchPin, &xHigherPriorityTaskWoken);

    // Clear the interrupt
    // ...

    // End the interrupt handling
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void app_main() {
    // Create the switch queue
    switchQueue = xQueueCreate(1, sizeof(uint32_t));

    // Configure UART2
    uart_config_t uartConfig = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uartConfig);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);

    // Configure LED pin
    //gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Configure user switch pin
    //gpio_pad_select_gpio(USER_SWITCH_PIN);
    gpio_set_direction(USER_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(USER_SWITCH_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(USER_SWITCH_PIN, GPIO_INTR_POSEDGE);

    // Install the user switch interrupt service routine
    gpio_install_isr_service(0);
    gpio_isr_handler_add(USER_SWITCH_PIN, user_switch_isr_handler, (void *)USER_SWITCH_PIN);

    // Create Task 1
    xTaskCreate(task1, "Task 1", configMINIMAL_STACK_SIZE, NULL, 1, &task1Handle);

    // Create Task 2
    xTaskCreate(task2, "Task 2", configMINIMAL_STACK_SIZE, NULL, 2, &task2Handle);

    // Create Task 3
    xTaskCreate(task3, "Task 3", configMINIMAL_STACK_SIZE * 5, NULL, 3, &task3Handle);

    // Create Task 4
    xTaskCreate(task4, "Task 4", configMINIMAL_STACK_SIZE, NULL, 4, &task4Handle);
}
