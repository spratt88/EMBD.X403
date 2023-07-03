//#include <stdio.h>
//#include <stdlib.h>
//#include "esp_log.h"
//#include "sdkconfig.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2
#define TASK3_PRIORITY 3
#define TASK4_PRIORITY 4
#define USER_SWITCH_PIN 14

// Task handles
TaskHandle_t task1Handle, task2Handle, task3Handle, task4Handle;

// Binary semaphore
SemaphoreHandle_t semaphore;

// Task function for Task 1
void task1(void *pvParameters) {
    while (1) {
        // Print message indicating Task 1 is running
        printf("Tsk1-P1 <-\n");

        // Run for about 100 times with task delay of 1
        for (int i = 0; i < 100; i++) {
            // Delay for 1 tick
            vTaskDelay(1);
        }

        // Print message indicating Task 1 is about to block
        printf("Tsk1-P1 ->\n");

        // Block for 10 milliseconds using semaphore
        xSemaphoreTake(semaphore, pdMS_TO_TICKS(10));
    }
}

// Task function for Task 2
void task2(void *pvParameters) {
    while (1) {
        // Print message indicating Task 2 is running
        printf("Tsk2-P2 <-\n");

        // Run for about 10 ticks
        for (int i = 0; i < 10; i++) {
            // Delay for 1 tick
            vTaskDelay(1);
        }

        // Print message indicating Task 2 is about to block
        printf("Tsk2-P2 ->\n");

        // Block for 250 milliseconds using semaphore
        xSemaphoreTake(semaphore, pdMS_TO_TICKS(250));
    }
}

// Task function for Task 3
void task3(void *pvParameters) {
    while (1) {
        // Task 3 functionality here

        // Delay for 1 tick
        vTaskDelay(1);
    }
}

// Task function for Task 4
void task4(void *pvParameters) {
    while (1) {
        if (semaphore != NULL) {
            // Print message indicating Task 4 is running
            printf("\t\t\tTsk4-P4 <-\n");

            // Task 4 functionality here

            // Run for about 10 ticks
            for (int i = 0; i < 10; i++) {
                // Delay for 1 tick
                vTaskDelay(1);
            }

            // Print message indicating Task 4 is about to put itself into a blocked state
            printf("\t\t\tTsk4-P4 ->\n");

            // Block the semaphore indefinitely
            xSemaphoreTake(semaphore, portMAX_DELAY);
        } else {
            vTaskDelay(1);
        }
    }
}

// User switch interrupt service routine
void IRAM_ATTR user_switch_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Check the GPIO level to handle rising edge interrupt
    if (gpio_get_level(USER_SWITCH_PIN) == 1) {
        // Release the semaphore
        xSemaphoreGiveFromISR(semaphore, &xHigherPriorityTaskWoken);
    }

    // Clear the interrupt
    // ...

    // End the interrupt handling
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void app_main() {
    // Create a binary semaphore and take it initially
    semaphore = xSemaphoreCreateBinary();

    // Configure user switch pin
    gpio_set_direction(USER_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(USER_SWITCH_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(USER_SWITCH_PIN, GPIO_INTR_POSEDGE);

    // Install the user switch interrupt service routine
    gpio_install_isr_service(0);
    gpio_isr_handler_add(USER_SWITCH_PIN, user_switch_isr_handler, NULL);

    // Create Task 1
    xTaskCreate(task1, "Task 1", configMINIMAL_STACK_SIZE, NULL, TASK1_PRIORITY, &task1Handle);

    // Create Task 2
    xTaskCreate(task2, "Task 2", configMINIMAL_STACK_SIZE, NULL, TASK2_PRIORITY, &task2Handle);

    // Create Task 3
    xTaskCreate(task3, "Task 3", configMINIMAL_STACK_SIZE, NULL, TASK3_PRIORITY, &task3Handle);

    // Create Task 4
    xTaskCreate(task4, "Task 4", configMINIMAL_STACK_SIZE, NULL, TASK4_PRIORITY, &task4Handle);
}
