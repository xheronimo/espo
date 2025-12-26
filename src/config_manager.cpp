#include "config_manager.h"
#include "utils.h"
#include <nvs.h>
#include <nvs_flash.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "hardware_defs.h"

static constexpr const char* NVS_NS = "scada_cfg";

/* ---------- Valores por defecto ---------- */
static void applyDefaultConfig(void) {
    memset(&cfgApp, 0, sizeof(cfgApp));
    strcpy(cfgApp.deviceID, "WATER_TANK_001");
    cfgApp.tiempoTensiones   = 1000;
    cfgApp.tiempoCorrientes = 1000;
    cfgApp.tiempoNivel      = 2000;
    cfgApp.tiempoPresion    = 2000;
    cfgApp.tiempoTempHum    = 5000;
    cfgApp.tiempoDigitales  = 500;
    cfgApp.tiempoComprobacionRed = 10000;
    cfgApp.tiempoReconexEthernet = 15000;
    cfgApp.timeoutRespuesta = 2000;

    /* 3 tensiones de ejemplo */
    for (int i = 0; i < 3; ++i) {
        snprintf(cfgApp.tension[i].config.nombre, 64, "Tension_L%d", i+1);
        cfgApp.tension[i].config.direccionModbus = 1;
        cfgApp.tension[i].config.registro = 30001 + i;
        cfgApp.tension[i].config.scale = 0.1f;
        cfgApp.tension[i].config.offset = 0.0f;
        cfgApp.tension[i].config.min = 200.0f;
        cfgApp.tension[i].config.max = 250.0f;
        cfgApp.tension[i].config.activo = true;
        strcpy(cfgApp.tension[i].alarma.mensajeBajo, "ALARM: Tensión baja");
        strcpy(cfgApp.tension[i].alarma.mensajeAlto, "ALARM: Tensión alta");
    }

    cfgApp.sht11.activo = true;
    cfgApp.sht11.tempMin = -10.0f;
    cfgApp.sht11.tempMax = 50.0f;
    cfgApp.sht11.humMin  = 0.0f;
    cfgApp.sht11.humMax  = 100.0f;
}

/* ---------- Guardar en NVS ---------- */
bool saveConfig(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return false;

    DynamicJsonDocument doc(4096);
    doc["deviceID"]                = cfgApp.deviceID;
    doc["tiempoTensiones"]        = cfgApp.tiempoTensiones;
    doc["tiempoCorrientes"]       = cfgApp.tiempoCorrientes;
    doc["tiempoNivel"]            = cfgApp.tiempoNivel;
    doc["tiempoPresion"]          = cfgApp.tiempoPresion;
    doc["tiempoTempHum"]          = cfgApp.tiempoTempHum;
    doc["tiempoDigitales"]        = cfgApp.tiempoDigitales;
    doc["tiempoComprobacionRed"] = cfgApp.tiempoComprobacionRed;
    doc["tiempoReconexEthernet"]  = cfgApp.tiempoReconexEthernet;
    doc["timeoutRespuesta"]       = cfgApp.timeoutRespuesta;

    /* Tensiones (solo 3 para el ejemplo) */
    JsonArray tens = doc.createNestedArray("tension");
    for (int i = 0; i < 3; ++i) {
        JsonObject s = tens.createNestedObject();
        s["nombre"]           = cfgApp.tension[i].config.nombre;
        s["direccionModbus"]  = cfgApp.tension[i].config.direccionModbus;
        s["registro"]         = cfgApp.tension[i].config.registro;
        s["scale"]            = cfgApp.tension[i].config.scale;
        s["offset"]           = cfgApp.tension[i].config.offset;
        s["min"]              = cfgApp.tension[i].config.min;
        s["max"]              = cfgApp.tension[i].config.max;
        s["activo"]           = cfgApp.tension[i].config.activo;
        JsonObject alarmas = s.createNestedObject("configAlarmas");
        alarmas["enviarSMS"]       = true;
        alarmas["enviarTelegram"]  = true;
        alarmas["enviarServidor"]  = true;
        alarmas["tiempoEntreAlertas"] = 60000;
        JsonObject msgs = s.createNestedObject("mensajes");
        msgs["bajo"] = cfgApp.tension[i].alarma.mensajeBajo;
        msgs["alto"] = cfgApp.tension[i].alarma.mensajeAlto;
    }

    /* SHT31 */
    JsonObject sht = doc.createNestedObject("sht11");
    sht["activo"]   = cfgApp.sht11.activo;
    sht["tempMin"]  = cfgApp.sht11.tempMin;
    sht["tempMax"]  = cfgApp.sht11.tempMax;
    sht["humMin"]   = cfgApp.sht11.humMin;
    sht["humMax"]   = cfgApp.sht11.humMax;

    String json;
    serializeJson(doc, json);
    err = nvs_set_blob(h, "cfg", json.c_str(), json.length());
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}

/* ---------- Cargar de NVS ---------- */
bool loadConfig(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        logMsg("NVS vacío ? carga defaults");
        applyDefaultConfig();
        return saveConfig();
    }

    size_t required = 0;
    err = nvs_get_blob_size(h, "cfg", &required);
    if (err != ESP_OK || required == 0) {
        nvs_close(h);
        applyDefaultConfig();
        return saveConfig();
    }

    std::vector<uint8_t> buf(required);
    err = nvs_get_blob(h, "cfg", buf.data(), &required);
    nvs_close(h);
    if (err != ESP_OK) {
        applyDefaultConfig();
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError dErr = deserializeJson(doc, buf.data(), required);
    if (dErr) {
        logMsg("JSON corrupto ? defaults");
        applyDefaultConfig();
        return saveConfig();
    }

    strncpy(cfgApp.deviceID, doc["deviceID"] | "UNKNOWN", sizeof(cfgApp.deviceID)-1);
    cfgApp.tiempoTensiones   = doc["tiempoTensiones"] | 1000;
    cfgApp.tiempoCorrientes = doc["tiempoCorrientes"] | 1000;
    cfgApp.tiempoNivel      = doc["tiempoNivel"] | 2000;
    cfgApp.tiempoPresion    = doc["tiempoPresion"] | 2000;
    cfgApp.tiempoTempHum    = doc["tiempoTempHum"] | 5000;
    cfgApp.tiempoDigitales  = doc["tiempoDigitales"] | 500;
    cfgApp.tiempoComprobacionRed = doc["tiempoComprobacionRed"] | 10000;
    cfgApp.tiempoReconexEthernet = doc["tiempoReconexEthernet"] | 15000;
    cfgApp.timeoutRespuesta = doc["timeoutRespuesta"] | 2000;

    if (doc.containsKey("tension")) {
        JsonArray tens = doc["tension"];
        for (size_t i = 0; i < tens.size() && i < 3; ++i) {
            JsonObject s = tens[i];
            strncpy(cfgApp.tension[i].config.nombre, s["nombre"] | "", sizeof(cfgApp.tension[i].config.nombre)-1);
            cfgApp.tension[i].config.direccionModbus = s["direccionModbus"] | 1;
            cfgApp.tension[i].config.registro        = s["registro"] | (30001 + i);
            cfgApp.tension[i].config.scale           = s["scale"] | 0.1f;
            cfgApp.tension[i].config.offset          = s["offset"] | 0.0f;
            cfgApp.tension[i].config.min             = s["min"] | 200.0f;
            cfgApp.tension[i].config.max             = s["max"] | 250.0f;
            cfgApp.tension[i].config.activo          = s["activo"] | true;
            JsonObject msgs = s["mensajes"];
            strncpy(cfgApp.tension[i].alarma.mensajeBajo, msgs["bajo"] | "ALARM: Tensión baja", sizeof(cfgApp.tension[i].alarma.mensajeBajo)-1);
            strncpy(cfgApp.tension[i].alarma.mensajeAlto, msgs["alto"] | "ALARM: Tensión alta", sizeof(cfgApp.tension[i].alarma.mensajeAlto)-1);
        }
    }

    if (doc.containsKey("sht11")) {
        JsonObject sht = doc["sht11"];
        cfgApp.sht11.activo = sht["activo"] | true;
        cfgApp.sht11.tempMin = sht["tempMin"] | -10.0f;
        cfgApp.sht11.tempMax = sht["tempMax"] | 50.0f;
        cfgApp.sht11.humMin  = sht["humMin"]  | 0.0f;
        cfgApp.sht11.humMax  = sht["humMax"]  | 100.0f;
    }

    return true;
}

/* ---------- Wrapper ---------- */
bool initConfig(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    return loadConfig();
}
