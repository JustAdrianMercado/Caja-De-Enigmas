#include "RelojUtc.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

namespace {

// Igual que en ConexionAwsIot: antes de esta fecha el reloj no esta sincronizado.
constexpr int64_t PrimerSegundoDe2026 = 1767225600;  // 1 de enero de 2026, UTC.
constexpr uint16_t MilisegundosPorSegundo = 1000;
constexpr long MicrosegundosPorMilisegundo = 1000;
constexpr int AnioBaseDeTm = 1900;  // struct tm cuenta los anios desde 1900.
constexpr int PrimerMesDeTm = 1;    // y los meses desde 0.

}  // namespace

bool RelojUtc::escribirAhora(char* destino, size_t capacidad) {
    timeval ahora;
    if (gettimeofday(&ahora, nullptr) != 0) {
        return false;
    }
    const uint16_t milisegundos =
        static_cast<uint16_t>(ahora.tv_usec / MicrosegundosPorMilisegundo);
    return escribir(static_cast<int64_t>(ahora.tv_sec), milisegundos, destino, capacidad);
}

bool RelojUtc::escribir(int64_t segundosDesde1970, uint16_t milisegundos, char* destino,
                        size_t capacidad) {
    if (destino == nullptr || segundosDesde1970 < PrimerSegundoDe2026 ||
        milisegundos >= MilisegundosPorSegundo) {
        return false;
    }
    const time_t segundos = static_cast<time_t>(segundosDesde1970);
    // gmtime no es reentrante, pero el firmware corre en una sola tarea (loop()).
    const tm* fecha = gmtime(&segundos);
    if (fecha == nullptr) {
        return false;
    }
    const int escritos =
        snprintf(destino, capacidad, "%04d-%02d-%02dT%02d:%02d:%02d.%03uZ",
                 fecha->tm_year + AnioBaseDeTm, fecha->tm_mon + PrimerMesDeTm, fecha->tm_mday,
                 fecha->tm_hour, fecha->tm_min, fecha->tm_sec,
                 static_cast<unsigned int>(milisegundos));
    return escritos > 0 && static_cast<size_t>(escritos) < capacidad;
}
