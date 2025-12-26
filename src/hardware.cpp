// src/hardware.cpp - Implementación de hardware
#include "hardware.h"
#include "utils.h"
#include "digital.h"
#include "config.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledDisponible = false;

void iniciarPerifericos() {
    Wire.begin(I2C_SDA, I2C_SCL);
    inicializarEntradasDigitales();
}

void iniciarDisplay() {
    if (display.begin(SSD1306_SWITCHCAPVCC, ADDR_DISPLAY)) {
        oledDisponible = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("SCADA System");
        display.println("Iniciando...");
        display.display();
        logMsg("Display OLED iniciado correctamente");
    } else {
        oledDisponible = false;
        logMsg("Display OLED no detectado");
    }
}
