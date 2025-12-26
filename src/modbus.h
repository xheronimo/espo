// src/modbus.h - Archivo de definiciones de comunicación Modbus
#ifndef MODBUS_H
#define MODBUS_H

#include <Arduino.h>
#include <ModbusMaster.h>

extern ModbusMaster node;

void iniciarModbus();
bool leerRegistroEntrada(uint8_t slaveAddress, uint16_t registro, float &valorLeido);
bool escribirRegistroHolding(uint8_t slaveAddress, uint16_t registro, uint16_t valor);

#endif
