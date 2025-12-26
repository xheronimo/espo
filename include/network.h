#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

extern EventGroupHandle_t evgNet;   // flags: ETH_OK, WIFI_OK, AP_OK

bool networkInit(void);
void networkTask(void *pvParameters);
