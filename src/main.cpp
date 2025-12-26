#include "utils.h"
#include "network.h"
#include "cola.h"
#include "gsm.h"
#include "telegram.h"
#include "http_client.h"
#include "sensor.h"
#include "logger.h"
#include "hardware.h"
#include "watchdog.h"
#include "alarms.h"
#include "display.h"
#include "webserver.h"

extern GSMManager gsm;               // singleton defined in gsm.cpp

constexpr uint32_t WATCHDOG_TIMEOUT_S = 30;   // 30?s

extern "C" void app_main(void) {
    /* --------- Inicialización básica --------- */
    Serial.begin(115200);
    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
    esp_task_wdt_add(nullptr);                     // Watchdog del proceso principal

    initLog();                                     // Mutex + ring-buffer de logs
    initConfig();                                  // NVS + defaults
    iniciarPerifericos();                          // I2C, SPI, ADC, OLED

    /* --------- Red ---------- */
    networkInit();                                // AP + STA + Ethernet
    xTaskCreatePinnedToCore(networkTask, "NetMgr", 4096, nullptr,
                            4, nullptr, 0);

    /* --------- GSM ---------- */
    if (!gsm.begin()) {
        logMsg("[GSM] No disponible - reintentaremos más tarde");
    }

    /* --------- Cola ---------- */
    colaInit();                                   // Cola única de NotifMsg

    /* --------- Tareas FreeRTOS (prioridades altas) ---------- */
    xTaskCreatePinnedToCore(sensorReadTask,   "SensorRead",  6144, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(alarmEvalTask,    "AlarmEval",   5120, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(notifDispatcher,  "NotifDisp",   6144, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(httpClientTask,    "HttpCli",    5120, nullptr, 4, nullptr, 1);
    xTaskCreatePinnedToCore(telegramProcess,  "Telegram",   5120, nullptr, 5, nullptr, 0);
    xTaskCreatePinnedToCore(loggerTask,       "Logger",     4096, nullptr, 3, nullptr, 0);
    xTaskCreatePinnedToCore(displayTask,      "Display",    4096, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(webServerTask,    "WebSrv",     4096, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(watchdogTask,    "Watchdog",   3072, nullptr, 5, nullptr, 0);
}
