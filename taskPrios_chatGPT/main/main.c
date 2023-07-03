//#include <stdio.h>
//#include <stdlib.h>
//#include "esp_log.h"
//#include "sdkconfig.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

#define TASK1_PRIORITY 4
#define TASK2_PRIORITY 3
#define TASK3_PRIORITY 2
#define TASK4_PRIORITY 1
#define USER_SWITCH_PIN 14

// Task handles
TaskHandle_t task1Handle, task2Handle, task3Handle, task4Handle;

// Binary semaphores for each task
SemaphoreHandle_t task1Semaphore, task2Semaphore, task4Semaphore;

// Task function for Task 1
void task1(void *pvParameters) {
    while (1) {
        // Attempt to obtain the semaphore
        if (task1Semaphore != NULL) {
            // Print message indicating Task 1 is running
            printf("Tsk1-P1 <-\n");

            // Run for about 100 times with task delay of 1
            for (int i = 0; i < 100; i++) {
                // Delay for 1 tick
                vTaskDelay(1);
            }

            // Print message indicating Task 1 is blocking
            printf("Tsk1-P1 ->\n");

            // Block for 10 milliseonds
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// Task function for Task 2
void task2(void *pvParameters) {
    while (1) {
        // Attempt to obtain the semaphore
        if (task2Semaphore != NULL) {
            // Print message indicating Task 2 is running
            printf("\tTsk2-P2 <-\n");

            // Run for about 10 ticks
            for (int i = 0; i < 10; i++) {
                // Delay for 1 tick
                vTaskDelay(1);
            }

            // Print message indicating Task 2 is blocking
            printf("\tTsk2-P2 ->\n");

            // Block for 250 milliseconds
            vTaskDelay(pdMS_TO_TICKS(250));
        }
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
        // Attempt to obtain the semaphore
        if (task4Semaphore != NULL) {
            // Print message indicating Task 4 is running
            printf("\t\t\tTsk4-P4 <-\n");

            // Task 4 functionality here

            // Run for about 10 ticks
            for (int i = 0; i < 10; i++) {
                // Delay for 1 tick
                vTaskDelay(1);
            }

            // Print message indicating Task 4 is blocking
            printf("\t\t\tTsk4-P4 ->\n");

            // Block indefinitely
            xSemaphoreTake(task4Semaphore, portMAX_DELAY);
        }
    }
}

// User switch interrupt service routine
void IRAM_ATTR user_switch_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Check the GPIO level to handle rising edge interrupt
    if (gpio_get_level(USER_SWITCH_PIN) == 1) {
        // Release the semaphore for Task 4
        xSemaphoreGiveFromISR(task4Semaphore, &xHigherPriorityTaskWoken);
    }

    // Clear the interrupt
    // ...

    // End the interrupt handling
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void app_main() {
    // Create binary semaphores for each task
    task1Semaphore = xSemaphoreCreateBinary();
    task2Semaphore = xSemaphoreCreateBinary();
    task4Semaphore = xSemaphoreCreateBinary();

    // Configure user switch pin
    //gpio_pad_select_gpio(USER_SWITCH_PIN);
    gpio_set_direction(USER_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(USER_SWITCH_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(USER_SWITCH_PIN, GPIO_INTR_POSEDGE);

    // Install the user switch interrupt service routine
    gpio_install_isr_service(0);
    gpio_isr_handler_add(USER_SWITCH_PIN, user_switch_isr_handler, NULL);

    // Set task priorities
    vTaskPrioritySet(task1Handle, TASK1_PRIORITY);
    vTaskPrioritySet(task2Handle, TASK2_PRIORITY);
    vTaskPrioritySet(task3Handle, TASK3_PRIORITY);
    vTaskPrioritySet(task4Handle, TASK4_PRIORITY);

    // Create Task 1
    xTaskCreate(task1, "Task 1", configMINIMAL_STACK_SIZE, NULL, TASK1_PRIORITY, &task1Handle);

    // Create Task 2
    xTaskCreate(task2, "Task 2", configMINIMAL_STACK_SIZE, NULL, TASK2_PRIORITY, &task2Handle);

    // Create Task 3
    xTaskCreate(task3, "Task 3", configMINIMAL_STACK_SIZE, NULL, TASK3_PRIORITY, &task3Handle);

    // Create Task 4
    xTaskCreate(task4, "Task 4", configMINIMAL_STACK_SIZE, NULL, TASK4_PRIORITY, &task4Handle);
}
