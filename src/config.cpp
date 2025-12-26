// src/config.cpp - Implementación de configuración del sistema
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

AppConfig_t cfgApp;
ModbusConfig_t cfgModbus;
NumeroSMS_t numerosSMS[MAX_NUMEROS_SMS];

bool cargarConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Error montando SPIFFS");
        return false;
    }

    File file = SPIFFS.open("/config.json", "r");
    if (!file) {
        inicializarConfigPorDefecto();
        guardarConfig();
        return true;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Error leyendo configuración");
        return false;
    }

    // Cargar configuración
    strlcpy(cfgApp.deviceID, doc["deviceID"] | "SCADA001", sizeof(cfgApp.deviceID));
    cfgApp.reporteGlobalActivo = doc["reporteGlobalActivo"] | true;
    cfgApp.intervaloReporteGlobal = doc["intervaloReporteGlobal"] | 60000;
    
    // Cargar configuración Modbus
    cfgModbus.baud = doc["modbus"]["baud"] | 9600;
    cfgModbus.parity = doc["modbus"]["parity"] | 0;
    cfgModbus.stopBits = doc["modbus"]["stopBits"] | 1;
    cfgModbus.slaveID = doc["modbus"]["slaveID"] | 1;
    cfgModbus.timeout = doc["modbus"]["timeout"] | 1000;
    cfgModbus.reintentos = doc["modbus"]["reintentos"] | 3;

    // Cargar números SMS
    JsonArray smsArray = doc["numerosSMS"];
    for (int i = 0; i < MAX_NUMEROS_SMS && i < smsArray.size(); i++) {
        JsonObject obj = smsArray[i];
        strlcpy(numerosSMS[i].numero, obj["numero"] | "", sizeof(numerosSMS[i].numero));
        numerosSMS[i].activo = obj["activo"] | false;
    }

    return true;
}

bool guardarConfig() {
    JsonDocument doc;
    
    doc["deviceID"] = cfgApp.deviceID;
    doc["reporteGlobalActivo"] = cfgApp.reporteGlobalActivo;
    doc["intervaloReporteGlobal"] = cfgApp.intervaloReporteGlobal;
    
    JsonObject modbus = doc["modbus"].to<JsonObject>();
    modbus["baud"] = cfgModbus.baud;
    modbus["parity"] = cfgModbus.parity;
    modbus["stopBits"] = cfgModbus.stopBits;
    modbus["slaveID"] = cfgModbus.slaveID;
    modbus["timeout"] = cfgModbus.timeout;
    modbus["reintentos"] = cfgModbus.reintentos;
    
    JsonArray smsArray = doc["numerosSMS"].to<JsonArray>();
    for (int i = 0; i < MAX_NUMEROS_SMS; i++) {
        JsonObject obj = smsArray.add<JsonObject>();
        obj["numero"] = numerosSMS[i].numero;
        obj["activo"] = numerosSMS[i].activo;
    }

    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        return false;
    }
    
    serializeJson(doc, file);
    file.close();
    return true;
}

void inicializarConfigPorDefecto() {
    strlcpy(cfgApp.deviceID, "SCADA001", sizeof(cfgApp.deviceID));
    cfgApp.reporteGlobalActivo = true;
    cfgApp.intervaloReporteGlobal = 60000;
    cfgApp.tiempoTensiones = 1000;
    cfgApp.tiempoCorrientes = 1000;
    cfgApp.tiempoNivel = 2000;
    cfgApp.tiempoPresion = 2000;
    cfgApp.tiempoTempHum = 5000;
    cfgApp.tiempoDigitales = 500;
    cfgApp.tiempoComprobacionRed = 10000;
    cfgApp.tiempoReconexEthernet = 15000;
    cfgApp.timeoutRespuesta = 2000;

    cfgModbus.baud = 9600;
    cfgModbus.parity = 0;
    cfgModbus.stopBits = 1;
    cfgModbus.slaveID = 1;
    cfgModbus.timeout = 1000;
    cfgModbus.reintentos = 3;

    // Números SMS por defecto
    for (int i = 0; i < MAX_NUMEROS_SMS; i++) {
        strlcpy(numerosSMS[i].numero, "", sizeof(numerosSMS[i].numero));
        numerosSMS[i].activo = false;
    }
}
