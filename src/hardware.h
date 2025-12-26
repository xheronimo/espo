// src/hardware.h - Archivo de definiciones de hardware
#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define ADDR_DISPLAY 0x3C

extern Adafruit_SSD1306 display;
extern bool oledDisponible;

void iniciarPerifericos();
void iniciarDisplay();

#endif
