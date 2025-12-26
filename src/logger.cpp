#include "logger.h"
#include "utils.h"

void loggerTask(void *pvParameters) {
    const TickType_t period = pdMS_TO_TICKS(5000);
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        flushLogToSD();          // escribe todo lo que haya en el ring-buffer a la SD
        vTaskDelayUntil(&lastWake, period);
    }
}
