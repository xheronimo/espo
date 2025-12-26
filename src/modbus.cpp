// src/modbus.cpp - Implementación de comunicación Modbus
#include "modbus.h"
#include "config.h"
#include "utils.h"

ModbusMaster node;

void iniciarModbus() {
    Serial1.begin(cfgModbus.baud, SERIAL_8N1, RS485_RX, RS485_TX);
    node.begin(cfgModbus.slaveID, Serial1);
    logMsg("Modbus iniciado");
}

bool leerRegistroEntrada(uint8_t slaveAddress, uint16_t registro, float &valorLeido) {
    node.begin(slaveAddress, Serial1);
    
    for (uint8_t intento = 0; intento < cfgModbus.reintentos; intento++) {
        uint8_t result = node.readInputRegisters(registro, 1);
        
        if (result == node.ku8MBSuccess) {
            uint16_t rawValue = node.getResponseBuffer(0);
            valorLeido = rawValue;
            node.clearResponseBuffer();
            return true;
        }
        delay(100);
    }
    
    valorLeido = 0.0f;
    return false;
}

bool escribirRegistroHolding(uint8_t slaveAddress, uint16_t registro, uint16_t valor) {
    node.begin(slaveAddress, Serial1);
    
    for (uint8_t intento = 0; intento < cfgModbus.reintentos; intento++) {
        uint8_t result = node.writeSingleRegister(registro, valor);
        
        if (result == node.ku8MBSuccess) {
            node.clearResponseBuffer();
            return true;
        }
        delay(100);
    }
    
    return false;
}
