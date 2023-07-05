#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2
#define TASK3_PRIORITY 3
#define TASK4_PRIORITY 4

#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_DATA_BITS UART_DATA_8_BITS
#define UART_PARITY UART_PARITY_DISABLE
#define UART_STOP_BITS UART_STOP_BITS_1

#define UART_RX_PIN GPIO_NUM_4
#define UART_TX_PIN GPIO_NUM_5
#define LED_PIN GPIO_NUM_47
#define USER_SWITCH_PIN GPIO_NUM_14

SemaphoreHandle_t semaphore3;
SemaphoreHandle_t semaphore4;

void task1(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t delayTicks = pdMS_TO_TICKS(10);

    while (1) {
        // Print message indicating Task 1 is running
        printf("Tsk1-P1 <-\n");

        // Run for about 100 ticks without using a task delay
        TickType_t startTick = xTaskGetTickCount();
        TickType_t currentTick = startTick;
        while (currentTick - startTick < 100) {
            // Perform Task 1's work

            // Allow other tasks to run
            taskYIELD();

            // Update current tick count
            currentTick = xTaskGetTickCount();
        }

        // Print message indicating Task 1 is about to block
        printf("Tsk1-P1 ->\n");

        // Block for 10 milliseconds
        vTaskDelayUntil(&lastWakeTime, delayTicks);
    }
}

void task2(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t delayTicks = pdMS_TO_TICKS(250);

    while (1) {
        // Print message indicating Task 2 is running
        printf("\tTsk2-P2 <-\n");

        // Run for about 10 ticks without using a task delay
        TickType_t startTick = xTaskGetTickCount();
        TickType_t currentTick = startTick;
        while (currentTick - startTick < 10) {
            // Perform Task 2's work

            // Allow other tasks to run
            taskYIELD();

            // Update current tick count
            currentTick = xTaskGetTickCount();
        }

        // Print message indicating Task 2 is about to block
        printf("\tTsk2-P2 ->\n");

        // Block for 250 milliseconds
        vTaskDelayUntil(&lastWakeTime, delayTicks);
    }
}

void task3(void *pvParameters) {
    char receivedChar;

    while (1) {
        // Wait for semaphore handle to be available
        if (xSemaphoreTake(semaphore3, portMAX_DELAY) == pdTRUE) {
            // Print message indicating Task 3 is running
            printf("\t\tTsk3-P3 <-\n");

            // Read character from UART
            if (uart_read_bytes(UART_PORT_NUM, (uint8_t*)&receivedChar, 1, 0) == 1) {
                // Print received character to log
                ESP_LOGI("Task3", "Received character: %c", receivedChar);

                // Toggle LED if 'L' or 'l' is pressed
                if (receivedChar == 'L' || receivedChar == 'l') {
                    gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
                }
            }

            // Print message indicating Task 3 is about to block
            printf("\t\tTsk3-P3 ->\n");

            // Allow other tasks to run
            taskYIELD();
        }
    }
}

void task4(void *pvParameters) {
    while (1) {
        // Wait for semaphore handle to be available
        if (xSemaphoreTake(semaphore4, portMAX_DELAY) == pdTRUE) {
            // Print message indicating Task 4 is running
            printf("\t\t\tTsk4-P4 <-\n");

            // Run for about 10 ticks without using a task delay
            TickType_t startTick = xTaskGetTickCount();
            TickType_t currentTick = startTick;
            while (currentTick - startTick < 10) {
                // Perform Task 4's work

                // Allow other tasks to run
                taskYIELD();

                // Update current tick count
                currentTick = xTaskGetTickCount();
            }

            // Print message indicating Task 4 is about to block
            printf("\t\t\tTsk4-P4 ->\n");

            // Allow other tasks to run
            taskYIELD();
        }
    }
}

void uart_console_isr(void *arg) {
    // Clear interrupt status
    uart_clear_intr_status(UART_PORT_NUM, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);

    // Check if there are received bytes
    if (uart_get_rx_fifo_len(UART_PORT_NUM) > 0) {
        // Release semaphore handle for task3
        xSemaphoreGiveFromISR(semaphore3, NULL);
    }
}

void user_switch_isr(void *arg) {
    // Clear interrupt status
    gpio_intr_clear(USER_SWITCH_PIN);

    // Release semaphore handle for task4
    xSemaphoreGiveFromISR(semaphore4, NULL);
}

void app_main() {
    // Create semaphores
    semaphore3 = xSemaphoreCreateBinary();
    semaphore4 = xSemaphoreCreateBinary();

    // Configure UART1
    uart_config_t uartConfig = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_BITS,
        .parity = UART_PARITY,
        .stop_bits = UART_STOP_BITS,
    };
    uart_param_config(UART_PORT_NUM, &uartConfig);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, 256, 0, 0, NULL, 0);

    // Configure LED pin
    //gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Configure UART interrupt
    uart_enable_rx_intr(UART_PORT_NUM);
    uart_isr_register(UART_PORT_NUM, uart_console_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);

    // Configure user switch interrupt
    gpio_install_isr_service(0);
    gpio_set_intr_type(USER_SWITCH_PIN, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(USER_SWITCH_PIN, user_switch_isr, NULL);

    // Create Task 1
    xTaskCreate(task1, "Task 1", configMINIMAL_STACK_SIZE, NULL, TASK1_PRIORITY, NULL);

    // Create Task 2
    xTaskCreate(task2, "Task 2", configMINIMAL_STACK_SIZE, NULL, TASK2_PRIORITY, NULL);

    // Create Task 3
    xTaskCreate(task3, "Task 3", configMINIMAL_STACK_SIZE, NULL, TASK3_PRIORITY, NULL);

    // Create Task 4
    xTaskCreate(task4, "Task 4", configMINIMAL_STACK_SIZE, NULL, TASK4_PRIORITY, NULL);
}

