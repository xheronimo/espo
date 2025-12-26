#include "cola.h"
#include "utils.h"
#include "config_manager.h"
#include "sht_simple.h"
#include <math.h>

static void generarAlarma(const char *texto, TipoAlarma tipo) {
    NotifMsg n{};
    strncpy(n.mensaje, texto, sizeof(n.mensaje)-1);
    n.destino = Destino::ALL;
    n.tipo    = tipo;
    n.timestamp = millis();
    colaEnqueue(n);
}

void evaluarAlarmasGenerales(void) {
    /* Nivel del depósito */
    if (cfgApp.nivel.activo) {
        if (cfgApp.nivel.estado.valor < cfgApp.nivel.config.min) {
            generarAlarma("?? Nivel bajo", TipoAlarma::BAJO);
        } else if (cfgApp.nivel.estado.valor > cfgApp.nivel.config.max) {
            generarAlarma("?? Nivel alto", TipoAlarma::ALTO);
        }
    }

    /* SHT31 */
    if (cfgApp.sht11.activo && !cfgApp.sht11.error) {
        if (cfgApp.sht11.temperatura < cfgApp.sht11.tempMin ||
            cfgApp.sht11.temperatura > cfgApp.sht11.tempMax) {
            generarAlarma("?? Temperatura fuera de rango", TipoAlarma::ALARMA_SHT_TEMP);
        }
        if (cfgApp.sht11.humedad < cfgApp.sht11.humMin ||
            cfgApp.sht11.humedad > cfgApp.sht11.humMax) {
            generarAlarma("?? Humedad fuera de rango", TipoAlarma::ALARMA_SHT_HUM);
        }
    }
}
