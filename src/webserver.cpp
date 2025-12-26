#include "webserver.h"
#include "utils.h"
#include <WebServer.h>

static WebServer server(80);

static void handleRoot() {
    server.send(200, "text/plain", "SCADA Water Tank - OK");
}
static void handleEstado() {
    String txt = "ID: " + String(cfgApp.deviceID) + "\n";
    txt += "Hora: " + getHoraRTC() + "\n";
    txt += "Nivel: " + String(cfgApp.nivel.estado.valor,1) + "%\n";
    txt += "Temp: " + String(cfgApp.sht11.temperatura,1) + "C\n";
    txt += "Hum:  " + String(cfgApp.sht11.humedad,1) + "%\n";
    server.send(200, "text/plain", txt);
}
static void notFound() {
    server.send(404, "text/plain", "Not found");
}

/* ---------- Init ---------- */
void setupWebServer(void) {
    server.on("/", handleRoot);
    server.on("/estado", handleEstado);
    server.onNotFound(notFound);
    server.begin();
    logMsg("[HTTP] Servidor web iniciado en puerto 80");
}

/* ---------- Loop (tarea) ---------- */
void webServerTask(void *pvParameters) {
    for (;;) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
