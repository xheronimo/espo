#pragma once
#include <stdbool.h>

void iniciarModbus(void);
bool leerRegistroEntrada(uint8_t slave, uint16_t registro, float &valor);
