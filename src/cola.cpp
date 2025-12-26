#include "cola.h"
#include "utils.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static QueueHandle_t xQueueNotif = nullptr;
static constexpr size_t QUEUE_LEN = 20;
static uint8_t queueStorage[QUEUE_LEN * sizeof(NotifMsg)];

void colaInit(void) {
    if (xQueueNotif) return;          // ya inicializada
    xQueueNotif = xQueueCreateStatic(
        QUEUE_LEN,
        sizeof(NotifMsg),
        queueStorage,
        nullptr);
}

/* Envío desde cualquier contexto (tarea o ISR) */
bool colaEnqueue(const NotifMsg& msg, TickType_t wait = 0) {
    return xQueueSend(xQueueNotif, &msg, wait) == pdTRUE;
}

/* Envío desde ISR (p.ej. GSM) */
bool colaEnqueueFromISR(const NotifMsg& msg, BaseType_t *pxHigherPriorityTaskWoken) {
    return xQueueSendFromISR(xQueueNotif, &msg, pxHigherPriorityTaskWoken) == pdTRUE;
}

/* Consumidor (tarea NotifDispatcher) */
bool colaDequeue(NotifMsg& msg, TickType_t wait = portMAX_DELAY) {
    return xQueueReceive(xQueueNotif, &msg, wait) == pdTRUE;
}

/* Número de elementos en la cola (para UI) */
size_t colaSize(void) {
    return uxQueueMessagesWaiting(xQueueNotif);
}
