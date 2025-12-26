#include "http_client.h"
#include "cola.h"
#include "utils.h"
#include "network.h"
#include <esp_http_client.h>

static const char *SERVER_URL = "http://example.com/api/measure";

/* ---------- Create client (uses active TCP/IP stack) ---------- */
static esp_http_client_handle_t crearCliente(void) {
    esp_http_client_config_t cfg = {
        .url = SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .disable_auto_redirect = true,
        .event_handler = nullptr,
        .crt_bundle_attach = nullptr,
        .transport_type = HTTP_TRANSPORT_OVER_TCP
    };
    return esp_http_client_init(&cfg);
}

/* ---------- Task that consumes the queue and sends ---------- */
void httpClientTask(void *pvParameters) {
    NotifMsg n;
    for (;;) {
        if (!colaDequeue(n, pdMS_TO_TICKS(200))) continue;
        if ((n.destino & Destino::SERVER) == Destino::NONE) continue;

        esp_http_client_handle_t client = crearCliente();
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, n.mensaje, strlen(n.mensaje));

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK && esp_http_client_get_status_code(client) == 200) {
            logMsg("[HTTP] JSON enviado OK");
        } else {
            logMsg("[HTTP] Fallo (%s)", esp_err_to_name(err));
            /* re-intento exponencial */
            n.intentos++;
            if (n.intentos < 4) {
                vTaskDelay(pdMS_TO_TICKS(500 * n.intentos));
                colaEnqueue(n);
            }
        }
        esp_http_client_cleanup(client);
    }
}
