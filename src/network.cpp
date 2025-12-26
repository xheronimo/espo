#include "network.h"
#include "utils.h"
#include <WiFi.h>
#include <Ethernet.h>
#include "hardware_defs.h"

EventGroupHandle_t evgNet = nullptr;
static constexpr uint32_t EV_ETH_OK   = (1 << 0);
static constexpr uint32_t EV_WIFI_OK  = (1 << 1);
static constexpr uint32_t EV_AP_OK    = (1 << 2);

/* ---------- Wi-Fi events ---------- */
static void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(evgNet, EV_WIFI_OK);
            logMsg("[WiFi] Conectado, IP %s", WiFi.localIP().toString().c_str());
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            xEventGroupClearBits(evgNet, EV_WIFI_OK);
            logMsg("[WiFi] Desconectado");
            break;
        default: break;
    }
}

/* ---------- Ethernet link monitor ---------- */
static void ethEvent(void) {
    if (Ethernet.linkStatus() == LinkON) {
        xEventGroupSetBits(evgNet, EV_ETH_OK);
        logMsg("[Ethernet] Conectado, IP %s", Ethernet.localIP().toString().c_str());
    } else {
        xEventGroupClearBits(evgNet, EV_ETH_OK);
        logMsg("[Ethernet] Link down");
    }
}

/* ---------- Init ---------- */
bool networkInit(void) {
    evgNet = xEventGroupCreate();

    /* 1) AP (siempre activo) */
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(cfgWifiAP.ssid.c_str(), cfgWifiAP.pass.c_str());
    xEventGroupSetBits(evgNet, EV_AP_OK);
    logMsg("[AP] SSID=%s", cfgWifiAP.ssid.c_str());

    /* 2) Ethernet (W5500) */
    Ethernet.init(ETH_CS_PIN);
    Ethernet.onLinkState([](bool up){ ethEvent(); });

    /* 3) Wi-Fi STA (fallback) */
    WiFi.onEvent(WiFiEvent);
    WiFi.begin(cfgWifi.ssid.c_str(), cfgWifi.pass.c_str());

    return true;
}

/* ---------- Loop de monitor ---------- */
void networkTask(void *pvParameters) {
    for (;;) {
        /* Si Ethernet está activo, apagamos Wi-Fi para ahorrar energía */
        if (xEventGroupGetBits(evgNet) & EV_ETH_OK) {
            if (WiFi.status() == WL_CONNECTED) {
                WiFi.disconnect(true);
                xEventGroupClearBits(evgNet, EV_WIFI_OK);
                logMsg("[Network] Ethernet activo ? Wi-Fi apagado");
            }
        } else {
            /* Re-intento Wi-Fi cada 10?s si no está conectado */
            static uint32_t lastAttempt = 0;
            if (millis() - lastAttempt > 10000) {
                lastAttempt = millis();
                if (WiFi.status() != WL_CONNECTED) {
                    logMsg("[Network] Re-intentando Wi-Fi...");
                    WiFi.reconnect();
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
