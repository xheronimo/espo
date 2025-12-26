#include "utils.h"
#include "sdcard.h"
#include <Wire.h>
#include "RTClib.h"

static RTC_DS3231 rtc;

/* ---------- RTC ---------- */
bool rtcDisponible(void) {
    return rtc.begin();
}
uint32_t getTimestamp(void) {
    if (rtcDisponible()) {
        DateTime now = rtc.now();
        return now.unixtime();          // segundos desde epoch
    }
    return millis() / 1000;            // fallback: uptime en segundos
}

/* ---------- Ring-buffer de log ---------- */
static const size_t LOG_BUF_SIZE = 2048;
static char   logRing[LOG_BUF_SIZE];
static size_t logHead = 0, logTail = 0;
static SemaphoreHandle_t xLogMutex = nullptr;

__attribute__((constructor))
static void initLogMutex(void) {
    xLogMutex = xSemaphoreCreateMutex();
}
void logMsg(const char *fmt, ...) {
    char line[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (xSemaphoreTake(xLogMutex, portMAX_DELAY) == pdTRUE) {
        size_t len = strlen(line);
        for (size_t i = 0; i <= len; ++i) {          // incluye '\0'
            logRing[logHead] = line[i];
            logHead = (logHead + 1) % LOG_BUF_SIZE;
            if (logHead == logTail)                 // sobrescribe la más vieja
                logTail = (logTail + 1) % LOG_BUF_SIZE;
        }
        xSemaphoreGive(xLogMutex);
    }
    Serial.println(line);   // siempre imprimir por serial (debug)
}
void flushLogToSD(void) {
    if (xSemaphoreTake(xLogMutex, portMAX_DELAY) != pdTRUE) return;
    if (logHead == logTail) { xSemaphoreGive(xLogMutex); return; }

    File f = SD.open(LOG_FILE, FILE_APPEND);
    if (!f) { xSemaphoreGive(xLogMutex); return; }

    while (logTail != logHead) {
        f.write(logRing[logTail]);
        logTail = (logTail + 1) % LOG_BUF_SIZE;
    }
    f.close();
    xSemaphoreGive(xLogMutex);
}

/* ---------- Formateo de hora ---------- */
String getHoraRTC(void) {
    if (rtcDisponible()) {
        DateTime now = rtc.now();
        char buf[20];
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d",
                 now.day(), now.month(), now.year(),
                 now.hour(), now.minute(), now.second());
        return String(buf);
    }
    unsigned long s = millis() / 1000;
    unsigned long h = s / 3600;
    unsigned long m = (s % 3600) / 60;
    unsigned long sec = s % 60;
    char buf[12];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, sec);
    return String(buf);
}

/* ---------- Helper: retry con back-off exponencial ---------- */
template<class Fn>
bool RetryWithBackoff(Fn fn, uint8_t maxAttempts = 5, uint32_t baseDelayMs = 200) {
    for (uint8_t i = 0; i < maxAttempts; ++i) {
        if (fn()) return true;
        vTaskDelay(pdMS_TO_TICKS(baseDelayMs * (i + 1)));
    }
    return false;
}
