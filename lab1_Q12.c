#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOSConfig.h"

#define UART_BUFFER_SIZE       1024
#define UART_LARGE_BUFFER_SIZE 2048
#define NO_DATA                0
#define SEMAPHORE_WAIT_TIME_MS 100
#define ONE_SECOND_DELAY_MS    1000
#define HALF_SECOND_DELAY_MS   500
#define GPIO_HIGH              1
#define GPIO_LOW               0

/* Global semaphore handle */
SemaphoreHandle_t binary_semaphore_handle;

/* Function to configure the UART */
void ConfigureUART(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_0, &uart_config); /* Configure UART with specified settings */
    uart_driver_install(UART_NUM_0, UART_LARGE_BUFFER_SIZE, NO_DATA, NO_DATA, NULL, NO_DATA);
}

/* Function to configure GPIO */
void ConfigureGPIO(void)
{
    gpio_config_t gpio_config = {
        .mode         = GPIO_MODE_DEF_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << GPIO_NUM_2) /* Configure GPIO2 */
    };
    gpio_config(&gpio_config); /* Apply GPIO configuration */
}

/* Task 1: Turn LED on */
void TaskTurnLEDOn(void *pv_parameters)
{
    while (1)
    {
        if (xSemaphoreTake(binary_semaphore_handle, (TickType_t) SEMAPHORE_WAIT_TIME_MS) == pdTRUE)
        {
            /* Set GPIO2 high to turn LED on */
            gpio_set_level(GPIO_NUM_2, GPIO_HIGH);
            vTaskDelay(HALF_SECOND_DELAY_MS / portTICK_PERIOD_MS); /* Delay for 500ms */
            xSemaphoreGive(binary_semaphore_handle);
        }
        else
        {
            printf("Semaphore not available\n"); /* Print message if semaphore is not available */
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Small delay to prevent tight looping
    }
}

/* Task 2: Turn LED off */
void TaskTurnLEDOff(void *pv_parameters)
{
    while (1)
    {
        if (xSemaphoreTake(binary_semaphore_handle, (TickType_t) SEMAPHORE_WAIT_TIME_MS) == pdTRUE)
        {
            /* Set GPIO2 low to turn LED off */
            gpio_set_level(GPIO_NUM_2, GPIO_LOW);
            vTaskDelay(ONE_SECOND_DELAY_MS / portTICK_PERIOD_MS); /* Delay for 1 second */
            xSemaphoreGive(binary_semaphore_handle);
        }
        else
        {
            printf("Semaphore not available\n");
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Small delay to prevent tight looping
    }
}

/* Task 3: Print a status message */
void TaskPrintStatus(void *pv_parameters)
{
    while (1)
    {
        /* Print status message via UART */
        printf("Task 3: Status message\n");
        vTaskDelay(ONE_SECOND_DELAY_MS / portTICK_PERIOD_MS); /* Delay for 1 second */
    }
}

/* Main application entry point */
void app_main(void)
{
    ConfigureGPIO(); /* Configure GPIO */
    ConfigureUART(); /* Configure UART */

    const int task_priority_1 = 1; // Task 1 priority
    const int task_priority_2 = 2; // Task 2 priority
    const int task_priority_3 = 3; // Task 3 priority

    binary_semaphore_handle = xSemaphoreCreateBinary(); /* Create a binary semaphore */

    if (binary_semaphore_handle != NULL)
    {
        xSemaphoreGive(binary_semaphore_handle); // Give the semaphore initially to allow access

        /* Create tasks with different priorities */
        xTaskCreate(TaskTurnLEDOn, "Task Turn LED On", UART_LARGE_BUFFER_SIZE, NULL, task_priority_1, NULL);
        xTaskCreate(TaskTurnLEDOff, "Task Turn LED Off", UART_LARGE_BUFFER_SIZE, NULL, task_priority_2, NULL);
        xTaskCreate(TaskPrintStatus, "Task Print Status", UART_LARGE_BUFFER_SIZE, NULL, task_priority_3, NULL);
    }
}
