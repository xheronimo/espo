#include "sensor.h"
#include "modbus.h"
#include "cola.h"
#include "utils.h"
#include "sht_simple.h"
#include <math.h>

static uint32_t ultimoEnvioJSON = 0;
static constexpr uint32_t PERIODO_JSON_MS = 5000;   // cada 5?s enviamos JSON completo

struct SensorData {
    uint32_t timestamp;
    float    nivel;
    float    temp;
    float    hum;
    float    tension[3];
    float    intensidad[3];
    float    presion[3];
};

/* ---------- Helper: crear JSON ---------- */
static void crearJSON(const SensorData &d, char *outBuf, size_t outSize) {
    DynamicJsonDocument doc(2048);
    doc["deviceID"] = cfgApp.deviceID;
    doc["ts"]       = d.timestamp;

    JsonArray t = doc.createNestedArray("tension");
    for (int i = 0; i < 3; ++i) t.add(d.tension[i]);

    JsonArray i = doc.createNestedArray("intensidad");
    for (int i = 0; i < 3; ++i) i.add(d.intensidad[i]);

    JsonArray p = doc.createNestedArray("presion");
    for (int i = 0; i < 3; ++i) p.add(d.presion[i]);

    doc["nivel"] = d.nivel;
    doc["temp"]  = d.temp;
    doc["hum"]   = d.hum;

    serializeJson(doc, outBuf, outSize);
}

/* ---------- Lectura de todos los sensores (no bloqueante) ---------- */
static void leerTodoSensorData(SensorData &sd) {
    /* Tensiones */
    for (int i = 0; i < 3; ++i) {
        float v;
        if (leerRegistroEntrada(cfgApp.tension[i].config.direccionModbus,
                                 cfgApp.tension[i].config.registro, v)) {
            sd.tension[i] = v * cfgApp.tension[i].config.scale + cfgApp.tension[i].config.offset;
        }
    }

    /* Intensidades */
    for (int i = 0; i < 3; ++i) {
        float v;
        if (leerRegistroEntrada(cfgApp.intensidad[i].config.direccionModbus,
                                 cfgApp.intensidad[i].config.registro, v)) {
            sd.intensidad[i] = v * cfgApp.intensidad[i].config.scale + cfgApp.intensidad[i].config.offset;
        }
    }

    /* Presiones */
    for (int i = 0; i < 3; ++i) {
        float v;
        if (leerRegistroEntrada(cfgApp.presion[i].config.direccionModbus,
                                 cfgApp.presion[i].config.registro, v)) {
            sd.presion[i] = v * cfgApp.presion[i].config.scale + cfgApp.presion[i].config.offset;
        }
    }

    /* Nivel (registro 40001 de ejemplo) */
    float nivelRaw;
    if (leerRegistroEntrada(2, 40001, nivelRaw)) {
        sd.nivel = nivelRaw * cfgApp.nivel.config.scale + cfgApp.nivel.config.offset;
    }

    /* SHT31 */
    if (shtDisponible()) {
        sd.temp = sht.readTemperature();
        sd.hum  = sht.readHumidity();
    } else {
        sd.temp = cfgApp.sht11.temperatura;
        sd.hum  = cfgApp.sht11.humedad;
    }

    sd.timestamp = getTimestamp();
}

/* ---------- Tarea que lee sensores y envía a la cola ---------- */
void sensorReadTask(void *pvParameters) {
    const TickType_t period = pdMS_TO_TICKS(500);   // 2?Hz global
    TickType_t lastWake = xTaskGetTickCount();

    SensorData data{};
    char jsonBuf[256];
    for (;;) {
        leerTodoSensorData(data);

        /* Enviar JSON completo cada PERIODO_JSON_MS */
        if (millis() - ultimoEnvioJSON >= PERIODO_JSON_MS) {
            crearJSON(data, jsonBuf, sizeof(jsonBuf));
            NotifMsg n{};
            strncpy(n.mensaje, jsonBuf, sizeof(n.mensaje)-1);
            n.destino = Destino::SERVER;
            n.tipo    = TipoAlarma::NINGUNA;
            n.timestamp = millis();
            colaEnqueue(n);
            ultimoEnvioJSON = millis();
        }

        /* Alarmas de nivel (ejemplo) */
        if (data.nivel < cfgApp.nivel.config.min) {
            NotifMsg n{};
            snprintf(n.mensaje, sizeof(n.mensaje),
                     "?? Nivel bajo: %.2f%%", data.nivel);
            n.destino = Destino::ALL;
            n.tipo    = TipoAlarma::BAJO;
            n.timestamp = millis();
            colaEnqueue(n);
        } else if (data.nivel > cfgApp.nivel.config.max) {
            NotifMsg n{};
            snprintf(n.mensaje, sizeof(n.mensaje),
                     "?? Nivel alto: %.2f%%", data.nivel);
            n.destino = Destino::ALL;
            n.tipo    = TipoAlarma::ALTO;
            n.timestamp = millis();
            colaEnqueue(n);
        }

        vTaskDelayUntil(&lastWake, period);
    }
}
