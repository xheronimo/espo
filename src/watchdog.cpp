#include "watchdog.h"
#include "utils.h"
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>

void watchdogTask(void *pvParameters) {
    const TickType_t period = pdMS_TO_TICKS(2000);   // alimentación cada 2?s
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWake, period);
    }
}
