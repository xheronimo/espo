#pragma once
#include <HardwareSerial.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class GSMManager {
public:
    explicit GSMManager(HardwareSerial &serial);
    bool begin(void);
    bool sendSMS(const char *num, const char *msg);
    bool isReady(void) const { return ready; }
private:
    HardwareSerial &port;
    SemaphoreHandle_t xMutex;
    bool ready = false;
    char lastError[64];
    bool sendAT(const char *cmd, const char *expected, uint32_t timeout);
};
