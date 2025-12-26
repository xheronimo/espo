// src/main.cpp - Archivo principal del sistema SCADA
// main.cpp - SCADA para depósito de agua con ESP32 y FreeRTOS
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"
#include "hardware.h"
#include "digital.h"
#include "datalogger.h"
#include "webserver.h"
#include "network.h"
#include "telegram.h"
#include "utils.h"
#include "modbus.h"
#include "sdcard.h"
#include "sensor.h"
#include "cola.h"
#include <esp_task_wdt.h>
#include "gsm.h"

// Definiciones de tareas
#define TASK_PRIORITY_HIGH 5
#define TASK_PRIORITY_MEDIUM 3
#define TASK_PRIORITY_LOW 1

#define STACK_SIZE_DEFAULT 4096
#define STACK_SIZE_SENSOR 8192
#define STACK_SIZE_COMMUNICATION 6144

// Handles de tareas
TaskHandle_t taskSensorHandle = NULL;
TaskHandle_t taskCommunicationHandle = NULL;
TaskHandle_t taskWebServerHandle = NULL;
TaskHandle_t taskDisplayHandle = NULL;
TaskHandle_t taskGSMHandle = NULL;
TaskHandle_t taskControlHandle = NULL;

// Colas de comunicación entre tareas
QueueHandle_t queueSensorData = NULL;
QueueHandle_t queueAlarmData = NULL;
QueueHandle_t queueControlData = NULL;

// Semáforos para sincronización
SemaphoreHandle_t semModbus = NULL;
SemaphoreHandle_t semSDCard = NULL;
SemaphoreHandle_t semDisplay = NULL;

// Estructura para datos de sensores
typedef struct {
    float nivel;
    float presiones[5];
    float tensiones[3];
    float corrientes[3];
    float temperatura;
    float humedad;
    bool motores[2];
    uint32_t timestamp;
} SensorData_t;

// Estructura para alarmas
typedef struct {
    char mensaje[256];
    uint8_t tipo;
    uint32_t timestamp;
} AlarmData_t;

// Estructura para control
typedef struct {
    uint8_t comando;
    uint8_t motor;
    float velocidad;
} ControlData_t;

// Variables globales
static SensorData_t sensorData = {0};
static bool systemReady = false;
static uint32_t lastSensorUpdate = 0;
static uint32_t lastCommunication = 0;

// Función para inicializar recursos del sistema
void initSystemResources() {
    // Crear colas
    queueSensorData = xQueueCreate(10, sizeof(SensorData_t));
    queueAlarmData = xQueueCreate(20, sizeof(AlarmData_t));
    queueControlData = xQueueCreate(5, sizeof(ControlData_t));
    
    // Crear semáforos
    semModbus = xSemaphoreCreateMutex();
    semSDCard = xSemaphoreCreateMutex();
    semDisplay = xSemaphoreCreateMutex();
    
    if (!queueSensorData || !queueAlarmData || !queueControlData ||
        !semModbus || !semSDCard || !semDisplay) {
        logMsg("Error creando recursos del sistema");
        return;
    }
}

// Tarea de lectura de sensores
void taskSensor(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // Cada segundo
    
    while (1) {
        // Leer sensor de nivel por RS485
        if (xSemaphoreTake(semModbus, pdMS_TO_TICKS(1000)) == pdTRUE) {
            float nivel = 0;
            if (leerSensorNivel(&nivel)) {
                sensorData.nivel = nivel;
            }
            xSemaphoreGive(semModbus);
        }
        
        // Leer sensores de presión (4-20mA)
        for (int i = 0; i < 5; i++) {
            float presion = leerSensorAnalogico(i);
            sensorData.presiones[i] = presion;
        }
        
        // Leer medidor de potencia trifásico
        if (xSemaphoreTake(semModbus, pdMS_TO_TICKS(1000)) == pdTRUE) {
            leerMedidorPotencia(sensorData.tensiones, sensorData.corrientes);
            xSemaphoreGive(semModbus);
        }
        
        // Leer sensor SHT31
        leerSHT31(&sensorData.temperatura, &sensorData.humedad);
        
        // Leer estado de motores
        sensorData.motores[0] = leerEstadoMotor(0);
        sensorData.motores[1] = leerEstadoMotor(1);
        
        sensorData.timestamp = millis();
        lastSensorUpdate = millis();
        
        // Enviar datos a cola de comunicación
        xQueueSend(queueSensorData, &sensorData, 0);
        
        // Verificar alarmas
        verificarAlarmas(&sensorData);
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Tarea de comunicación
void taskCommunication(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); // Cada 5 segundos
    
    while (1) {
        SensorData_t data;
        AlarmData_t alarm;
        
        // Procesar datos de sensores
        if (xQueueReceive(queueSensorData, &data, 0) == pdTRUE) {
            // Enviar a servidor JSON
            enviarDatosServidor(&data);
            
            // Guardar en SD
            if (xSemaphoreTake(semSDCard, pdMS_TO_TICKS(1000)) == pdTRUE) {
                guardarDatosSD(&data);
                xSemaphoreGive(semSDCard);
            }
        }
        
        // Procesar alarmas
        if (xQueueReceive(queueAlarmData, &alarm, 0) == pdTRUE) {
            // Enviar SMS
            enviarSMSAlarma(&alarm);
            
            // Enviar Telegram
            enviarTelegramAlarma(&alarm);
        }
        
        // Reintentar conexiones fallidas
        reintentarConexiones();
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Tarea de servidor web
void taskWebServer(void *pvParameters) {
    setupWebServer();
    
    while (1) {
        loopWebServer();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarea de display OLED
void taskDisplay(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(2000); // Cada 2 segundos
    
    while (1) {
        if (xSemaphoreTake(semDisplay, pdMS_TO_TICKS(1000)) == pdTRUE) {
            actualizarDisplay(&sensorData);
            xSemaphoreGive(semDisplay);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Tarea de GSM
void taskGSM(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10000); // Cada 10 segundos
    
    // Inicializar GSM
    if (!iniciarGSM()) {
        logMsg("Error inicializando GSM");
        vTaskDelete(NULL);
    }
    
    while (1) {
        // Procesar cola de SMS
        procesarColaSMS();
        
        // Verificar estado de red
        verificarEstadoGSM();
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Tarea de control de motores
void taskControl(void *pvParameters) {
    ControlData_t controlData;
    
    while (1) {
        // Recibir comandos de control
        if (xQueueReceive(queueControlData, &controlData, pdMS_TO_TICKS(1000)) == pdTRUE) {
            switch (controlData.comando) {
                case 1: // Arrancar motor
                    arrancarMotor(controlData.motor, controlData.velocidad);
                    break;
                case 2: // Parar motor
                    pararMotor(controlData.motor);
                    break;
                case 3: // Alternar motores
                    alternarMotores();
                    break;
            }
        }
        
        // Control automático basado en nivel
        controlAutomaticoMotores(&sensorData);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Función para verificar alarmas
void verificarAlarmas(SensorData_t *data) {
    static bool nivelBajoAlarma = false;
    static bool nivelAltoAlarma = false;
    
    // Verificar nivel de agua
    if (data->nivel < NIVEL_MINIMO && !nivelBajoAlarma) {
        AlarmData_t alarm;
        snprintf(alarm.mensaje, sizeof(alarm.mensaje), 
                "ALARMA: Nivel de agua bajo (%.2f%%)", data->nivel);
        alarm.tipo = 1; // Alarma de nivel
        alarm.timestamp = millis();
        
        xQueueSend(queueAlarmData, &alarm, 0);
        nivelBajoAlarma = true;
        nivelAltoAlarma = false;
    }
    else if (data->nivel > NIVEL_MAXIMO && !nivelAltoAlarma) {
        AlarmData_t alarm;
        snprintf(alarm.mensaje, sizeof(alarm.mensaje), 
                "ALARMA: Nivel de agua alto (%.2f%%)", data->nivel);
        alarm.tipo = 2; // Alarma de nivel alto
        alarm.timestamp = millis();
        
        xQueueSend(queueAlarmData, &alarm, 0);
        nivelAltoAlarma = true;
        nivelBajoAlarma = false;
    }
    else if (data->nivel >= NIVEL_MINIMO && data->nivel <= NIVEL_MAXIMO) {
        nivelBajoAlarma = false;
        nivelAltoAlarma = false;
    }
    
    // Verificar presiones
    for (int i = 0; i < 5; i++) {
        if (data->presiones[i] < PRESION_MINIMA || data->presiones[i] > PRESION_MAXIMA) {
            AlarmData_t alarm;
            snprintf(alarm.mensaje, sizeof(alarm.mensaje), 
                    "ALARMA: Presión %d fuera de rango (%.2f bar)", i+1, data->presiones[i]);
            alarm.tipo = 3; // Alarma de presión
            alarm.timestamp = millis();
            
            xQueueSend(queueAlarmData, &alarm, 0);
        }
    }
}

// Función para enviar datos al servidor
void enviarDatosServidor(SensorData_t *data) {
    // Prioridad: Ethernet > WiFi > 4G
    if (enviarPorEthernet(data)) {
        logMsg("Datos enviados por Ethernet");
    }
    else if (enviarPorWiFi(data)) {
        logMsg("Datos enviados por WiFi");
    }
    else if (enviarPor4G(data)) {
        logMsg("Datos enviados por 4G");
    }
    else {
        logMsg("Error enviando datos - todas las conexiones fallidas");
    }
}

// Función para enviar SMS de alarma
void enviarSMSAlarma(AlarmData_t *alarm) {
    // Enviar a lista de números configurados
    for (int i = 0; i < MAX_NUMEROS_SMS; i++) {
        if (numerosSMS[i].activo) {
            enviarSMS(numerosSMS[i].numero, alarm->mensaje);
        }
    }
}

// Función para enviar Telegram de alarma
void enviarTelegramAlarma(AlarmData_t *alarm) {
    enviarMensajeTelegram(alarm->mensaje);
}

// Función principal setup
void setup() {
    Serial.begin(115200);
    
    // Inicializar watchdog
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(NULL);
    
    logMsg("Iniciando sistema SCADA para depósito de agua...");
    
    // Inicializar recursos del sistema
    initSystemResources();
    
    // Inicializar hardware
    iniciarPerifericos();
    iniciarModbus();
    iniciarSD();
    iniciarDisplay();
    iniciarRed();
    
    // Crear tareas FreeRTOS
    xTaskCreate(taskSensor, "SensorTask", STACK_SIZE_SENSOR, NULL, 
                TASK_PRIORITY_HIGH, &taskSensorHandle);
    
    xTaskCreate(taskCommunication, "CommTask", STACK_SIZE_COMMUNICATION, NULL, 
                TASK_PRIORITY_MEDIUM, &taskCommunicationHandle);
    
    xTaskCreate(taskWebServer, "WebTask", STACK_SIZE_DEFAULT, NULL, 
                TASK_PRIORITY_MEDIUM, &taskWebServerHandle);
    
    xTaskCreate(taskDisplay, "DisplayTask", STACK_SIZE_DEFAULT, NULL, 
                TASK_PRIORITY_LOW, &taskDisplayHandle);
    
    xTaskCreate(taskGSM, "GSMTask", STACK_SIZE_DEFAULT, NULL, 
                TASK_PRIORITY_MEDIUM, &taskGSMHandle);
    
    xTaskCreate(taskControl, "ControlTask", STACK_SIZE_DEFAULT, NULL, 
                TASK_PRIORITY_HIGH, &taskControlHandle);
    
    systemReady = true;
    logMsg("Sistema iniciado correctamente");
}

// Función principal loop
void loop() {
    // El loop principal queda vacío ya que todo se maneja por tareas
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}
