#pragma once
#include <freertos/FreeRTOS.h>

bool telegramInit(void);
bool telegramSend(const char *cmsg);
void telegramEnableAlarms(bool on);
void telegramProcess(void *pvParameters);
