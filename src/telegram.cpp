#include "telegram.h"
#include "cola.h"
#include "utils.h"
#include "network.h"
#include <WiFiClientSecure.h>
#include "UniversalTelegramBot.h"

static WiFiClientSecure tgClient;
static UniversalTelegramBot bot(cfgTelegram.botToken, tgClient);
static uint32_t lastSent = 0;
static bool telegramAlarmasActivas = true;

/* ---------- Init ---------- */
bool telegramInit(void) {
    bot.updateToken(cfgTelegram.botToken);
    bot.setChatID(cfgTelegram.chatID);
    return true;
}

/* ---------- Send (rate-limit) ---------- */
bool telegramSend(const char *cmsg) {
    if (!telegramAlarmasActivas) return false;
    if (millis() - lastSent < cfgTelegram.rateLimitMs) return false;
    if (!(xEventGroupGetBits(evgNet) & (EV_ETH_OK | EV_WIFI_OK))) return false;

    /* Convertir a String solo para la librería, pero la longitud es <128 */
    String msg = String("[SCADA] ") + String(cmsg);
    bot.sendMessage(cfgTelegram.chatID, msg, "");
    lastSent = millis();
    return true;
}

/* ---------- Enable / Disable alarm notifications ---------- */
void telegramEnableAlarms(bool on) {
    telegramAlarmasActivas = on;
}

/* ---------- Dispatcher (task) ---------- */
void telegramProcess(void *pvParameters) {
    NotifMsg n;
    for (;;) {
        if (!colaDequeue(n, pdMS_TO_TICKS(100))) continue;

        /* Descarta alarmas desactivadas (excepto reporte global) */
        if (!telegramAlarmasActivas && n.tipo != TipoAlarma::REPORTE_GLOBAL) continue;

        char txt[144];
        snprintf(txt, sizeof(txt), "[%s] %s", getHoraRTC().c_str(), n.mensaje);
        if (!telegramSend(txt)) {
            /* re-intento exponencial */
            n.intentos++;
            if (n.intentos < 5) {
                vTaskDelay(pdMS_TO_TICKS(200 * n.intentos));
                colaEnqueue(n);
            }
        }
    }
}
