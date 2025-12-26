// src/config.h - Archivo de configuración del sistema
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Definiciones de hardware para KCK868A16V3
#define ANALOG_A1 4
#define ANALOG_A2 6
#define ANALOG_A3 7
#define ANALOG_A4 5
#define I2C_SDA 9
#define I2C_SCL 10
#define RS485_RX 17
#define RS485_TX 16
#define SD_MOSI 12
#define SD_SCK 13
#define SD_MISO 14
#define SD_CS 11
#define SD_CD 21
#define ETHERNET_CLK 42
#define ETHERNET_MOSI 43
#define ETHERNET_MISO 44
#define ETHERNET_CS 15
#define ETHERNET_INT 2
#define ETHERNET_RST 1

// Definiciones de umbrales
#define NIVEL_MINIMO 10.0
#define NIVEL_MAXIMO 90.0
#define PRESION_MINIMA 0.5
#define PRESION_MAXIMA 10.0
#define MAX_NUMEROS_SMS 5

// Estructuras de configuración
typedef struct {
    char numero[20];
    bool activo;
} NumeroSMS_t;

typedef struct {
    char deviceID[32];
    bool reporteGlobalActivo;
    uint32_t intervaloReporteGlobal;
    uint32_t tiempoTensiones;
    uint32_t tiempoCorrientes;
    uint32_t tiempoNivel;
    uint32_t tiempoPresion;
    uint32_t tiempoTempHum;
    uint32_t tiempoDigitales;
    uint32_t tiempoComprobacionRed;
    uint32_t tiempoReconexEthernet;
    uint32_t timeoutRespuesta;
} AppConfig_t;

typedef struct {
    uint32_t baud;
    uint8_t parity;
    uint8_t stopBits;
    uint8_t slaveID;
    uint32_t timeout;
    uint8_t reintentos;
} ModbusConfig_t;

extern AppConfig_t cfgApp;
extern ModbusConfig_t cfgModbus;
extern NumeroSMS_t numerosSMS[MAX_NUMEROS_SMS];

// Funciones
bool cargarConfig();
bool guardarConfig();
void inicializarConfigPorDefecto();

#endif
