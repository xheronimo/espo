#include "hardware.h"
#include "utils.h"
#include "digital.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void iniciarPerifericos(void) {
    Wire.begin(I2C_SDA, I2C_SCL);
    inicializarPCF();
    iniciarDisplay();
}
void iniciarDisplay(void) {
    if (display.begin(SSD1306_SWITCHCAPVCC, ADDR_DISPLAY)) {
        oledDisponible = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("Display OK");
        display.display();
    } else {
        logMsg("[OLED] No detectada");
        oledDisponible = false;
    }
}
