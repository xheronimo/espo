#pragma once
#include <cstddef>
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* ---------- Destinos (bitmask) ---------- */
enum class Destino : uint8_t {
    NONE      = 0,
    GSM       = 1 << 0,
    TELEGRAM  = 1 << 1,
    SERVER    = 1 << 2,
    ALL       = GSM | TELEGRAM | SERVER
};
inline Destino operator|(Destino a, Destino b){ return static_cast<Destino>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
inline Destino operator&(Destino a, Destino b){ return static_cast<Destino>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
inline Destino& operator|=(Destino& a, Destino b){ a = a | b; return a; }

/* ---------- Tipo de alarma ---------- */
enum class TipoAlarma : uint8_t {
    NINGUNA,
    BAJO,
    ALTO,
    NORMAL,
    INACTIVO,
    ERROR_LECTURA,
    REPORTE_PERIODICO,
    REPORTE_GLOBAL,
    ALARMA_SHT_TEMP,
    ALARMA_SHT_HUM
};

/* ---------- Mensaje unificado ---------- */
struct NotifMsg {
    char    mensaje[128];
    Destino destino;
    TipoAlarma tipo;
    char    sensorNombre[32];   // opcional, para trazabilidad
    uint32_t timestamp;        // milisegundos (RTC)
    uint8_t intentos;          // contador de reintentos
};

/* API de la cola */
void   colaInit(void);
bool   colaEnqueue(const NotifMsg& msg, TickType_t wait = 0);
bool   colaEnqueueFromISR(const NotifMsg& msg, BaseType_t *pxHigherPriorityTaskWoken);
bool   colaDequeue(NotifMsg& msg, TickType_t wait = portMAX_DELAY);
size_t colaSize(void);
