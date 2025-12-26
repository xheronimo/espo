#include "gsm.h"
#include "utils.h"
#include <HardwareSerial.h>

GSMManager::GSMManager(HardwareSerial &serial) : port(serial) {
    xMutex = xSemaphoreCreateMutex();
}

/* ---------- Inicialización ---------- */
bool GSMManager::begin(void) {
    port.begin(cfgSms.baudrate, SERIAL_8N1, cfgSms.txPin, cfgSms.rxPin);
    port.setTimeout(2000);
    vTaskDelay(pdMS_TO_TICKS(3000));

    if (!sendAT("AT", "OK", 3000)) {
        strncpy(lastError, "No response from GSM", sizeof(lastError)-1);
        logMsg("[GSM] No responde al AT");
        return false;
    }
    if (!sendAT("AT+CMGF=1", "OK", 3000)) {
        strncpy(lastError, "Cannot set SMS text mode", sizeof(lastError)-1);
        logMsg("[GSM] No se pudo cambiar a modo texto");
        return false;
    }

    ready = true;
    logMsg("[GSM] Módulo listo");
    return true;
}

/* ---------- Envío de SMS ---------- */
bool GSMManager::sendSMS(const char *num, const char *msg) {
    if (!ready) return false;
    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(2000)) != pdTRUE) return false;

    bool ok = false;
    port.printf("AT+CMGS=\"%s\"\r", num);
    if (port.find(">")) {
        port.print(msg);
        port.write(0x1A);                 // Ctrl-Z
        ok = port.find("OK");
    }
    xSemaphoreGive(xMutex);
    return ok;
}

/* ---------- AT command helper ---------- */
bool GSMManager::sendAT(const char *cmd, const char *expected, uint32_t timeout) {
    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(2000)) != pdTRUE) return false;
    port.println(cmd);
    bool resp = port.find(expected);
    xSemaphoreGive(xMutex);
    return resp;
}
