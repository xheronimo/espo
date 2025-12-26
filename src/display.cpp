#include "display.h"
#include "utils.h"
#include "hardware.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static TickType_t lastRefresh = 0;
static constexpr TickType_t REFRESH_MS = pdMS_TO_TICKS(1000);

void displayTask(void *pvParameters) {
    for (;;) {
        if (!oledDisponible) { vTaskDelay(REFRESH_MS); continue; }
        if (millis() - lastRefresh < 1000) { vTaskDelay(REFRESH_MS); continue; }
        lastRefresh = millis();

        display.clearDisplay();
        display.setCursor(0,0);
        display.printf("ID: %s\n", cfgApp.deviceID);
        display.printf("Hora: %s\n", getHoraRTC().c_str());
        display.printf("Nivel: %.1f%%\n", cfgApp.nivel.estado.valor);
        display.printf("Temp: %.1fC\n", cfgApp.sht11.temperatura);
        display.printf("Hum:  %.1f%%\n", cfgApp.sht11.humedad);
        display.display();

        vTaskDelay(REFRESH_MS);
    }
}
