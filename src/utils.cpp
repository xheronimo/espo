// src/utils.cpp - Implementación de utilidades del sistema
#include "utils.h"
#include <SPIFFS.h>

void logMsg(const String &msg) {
    Serial.println(msg);
    
    // Guardar en archivo de log
    File file = SPIFFS.open("/system.log", "a");
    if (file) {
        file.println(msg);
        file.close();
    }
}

String getHoraRTC() {
    // Implementar lectura de RTC
    return "00:00:00";
}
