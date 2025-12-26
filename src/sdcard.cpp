#include "sdcard.h"
#include "utils.h"
#include <SPI.h>
#include <SD.h>

bool iniciarSD(void) {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        logMsg("[SD] Error al iniciar");
        return false;
    }
    logMsg("[SD] Iniciada, tamaño %llu MB",
           (unsigned long long)(SD.totalBytes()/(1024ULL*1024ULL)));
    return true;
}

bool escribirLogSD(const String &msg) {
    File f = SD.open(LOG_FILE, FILE_APPEND);
    if (!f) return false;
    f.println(msg);
    f.close();
    return true;
}
