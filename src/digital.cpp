#include "digital.h"
#include "config_manager.h"
#include "utils.h"
#include <PCF8575.h>

static PCF8575 pcf_out1(PCF_OUT1_ADDR);
static PCF8575 pcf_out2(PCF_OUT2_ADDR);
static PCF8575 pcf_in1(PCF_IN1_ADDR);
static PCF8575 pcf_in2(PCF_IN2_ADDR);

/* ---------- Init PCF8575 ---------- */
void inicializarPCF(void) {
    if (!pcf_out1.begin() || !pcf_out2.begin() ||
        !pcf_in1.begin()  || !pcf_in2.begin()) {
        logMsg("[PCF8575] Error de inicialización");
    } else {
        logMsg("[PCF8575] OK");
    }

    for (int i = 0; i < 8; ++i) {
        pcf_out1.write(i, LOW);
        pcf_out2.write(i, LOW);
    }
}

/* ---------- Process digital inputs ---------- */
void procesarEntradasDigitales(void) {
    static uint32_t ultimo = 0;
    if (millis() - ultimo < cfgApp.tiempoDigitales) return;
    ultimo = millis();

    for (int i = 0; i < DI_MAX; ++i) {
        if (!cfgApp.entradas[i].activa) continue;

        bool actual;
        if (cfgApp.entradas[i].gpio < 8)
            actual = pcf_in1.read(cfgApp.entradas[i].gpio);
        else
            actual = pcf_in2.read(cfgApp.entradas[i].gpio - 8);

        if (actual != entradas.entradas[i].valor) {
            entradas.entradas[i].valor   = actual;
            entradas.entradas[i].cambio  = true;
            entradas.entradas[i].timestamp = getTimestamp();

            if (entradas.entradas[i].alarma) {
                const char *msg = actual ? cfgApp.entradas[i].mensajeOn
                                         : cfgApp.entradas[i].mensajeOff;
                NotifMsg n{};
                strncpy(n.mensaje, msg, sizeof(n.mensaje)-1);
                n.destino = Destino::ALL;
                n.tipo    = actual ? TipoAlarma::ALTO : TipoAlarma::BAJO;
                n.timestamp = millis();
                colaEnqueue(n);
                logMsg("[DI%u] %s", i, msg);
            }
        } else {
            entradas.entradas[i].cambio = false;
        }
    }
}
