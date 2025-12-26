#pragma once
#include <stdbool.h>

typedef struct {
    char    nombre[64];
    uint8_t direccionModbus;
    uint16_t registro;
    float   scale;
    float   offset;
    float   min;
    float   max;
    bool    activo;
} SensorConfig;

typedef struct {
    char    nombre[64];
    bool    activo;
    float   temperatura;
    float   humedad;
    float   tempMin;
    float   tempMax;
    float   humMin;
    float   humMax;
    bool    alarmaTemp;
    bool    alarmaHum;
    bool    error;
} SHT11Config;

typedef struct {
    char    deviceID[32];
    uint32_t tiempoTensiones;
    uint32_t tiempoCorrientes;
    uint32_t tiempoNivel;
    uint32_t tiempoPresion;
    uint32_t tiempoTempHum;
    uint32_t tiempoDigitales;
    uint32_t tiempoComprobacionRed;
    uint32_t tiempoReconexEthernet;
    uint32_t timeoutRespuesta;
    SensorConfig tension[3];
    SensorConfig intensidad[3];
    SensorConfig presion[3];
    struct {
        bool    activo;
        float   valor;
    } nivel;
    SHT11Config sht11;
} AppConfig;

extern AppConfig cfgApp;

/* API */
bool initConfig(void);
bool loadConfig(void);
bool saveConfig(void);
