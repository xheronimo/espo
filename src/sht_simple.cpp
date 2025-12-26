#include "sht_simple.h"
#include "utils.h"
#include <Adafruit_SHT31.h>

static Adafruit_SHT31 sht31 = Adafruit_SHT31();
static bool shtOK = false;

void inicializarSHT(void) {
    shtOK = sht31.begin(0x44);
    if (!shtOK) {
        logMsg("[SHT31] No detectado, usando valores simulados");
        cfgApp.sht11.activo = false;
        return;
    }
    logMsg("[SHT31] Detectado OK");
    cfgApp.sht11.activo = true;
}
bool shtDisponible(void) { return shtOK; }
float leerTemperatura(void) { return shtOK ? sht31.readTemperature() : NAN; }
float leerHumedad(void)     { return shtOK ? sht31.readHumidity()    : NAN; }
