#include "modbus.h"
#include "utils.h"
#include <ModbusMaster.h>

ModbusMaster node;

/* ---------- RS-485 control (DE/RE) ---------- */
static void preTransmission(void) { delayMicroseconds(50); }
static void postTransmission(void){ delayMicroseconds(50); }

void iniciarModbus(void) {
    Serial1.begin(cfgModbus.baud,
                 cfgModbus.parity == 0 ? SERIAL_8N1 :
                 (cfgModbus.parity == 1 ? SERIAL_8E1 : SERIAL_8O1),
                 RS485_RX, RS485_TX);
    node.begin(cfgModbus.slaveID, Serial1);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
}

/* ---------- Lectura de registro (input) ---------- */
bool leerRegistroEntrada(uint8_t slave, uint16_t registro, float &valor) {
    node.begin(slave, Serial1);
    for (uint8_t intento = 0; intento < cfgModbus.reintentos; ++intento) {
        uint8_t rc = node.readInputRegisters(registro, 1);
        if (rc == node.ku8MBSuccess && node.available()) {
            uint16_t raw = node.getResponseBuffer(0);
            valor = static_cast<float>(raw);
            node.clearResponseBuffer();
            return true;
        }
        logMsg("[Modbus] Fallo reg %u (slave %u) intento %u", registro, slave, intento+1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    valor = 0.0f;
    return false;
}
