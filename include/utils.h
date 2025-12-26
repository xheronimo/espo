#pragma once
#include <Arduino.h>

bool rtcDisponible(void);
uint32_t getTimestamp(void);
void logMsg(const char *fmt, ...) __attribute__((format(printf,1,2)));
void flushLogToSD(void);
String getHoraRTC(void);

/* Helper genérico de re-intento con back-off (C++11-compatible) */
template<class Fn>
bool RetryWithBackoff(Fn fn, uint8_t maxAttempts = 5, uint32_t baseDelayMs = 200);
