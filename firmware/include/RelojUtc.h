#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Hora UTC del ESP32 para el campo tsObjeto de los eventos (docs/contratos.md, seccion 3.2).
 *
 * La hora real llega por NTP cuando ConexionAwsIot se conecta. Antes de eso el reloj marca 1970,
 * y el evento se publica sin tsObjeto. Usa solo funciones de C, asi se prueba en la PC.
 */
class RelojUtc {
public:
    // Formato exacto que valida la Lambda de almacenamiento: "2026-11-21T15:20:03.120Z".
    static constexpr size_t TamanoTexto = sizeof("2026-11-21T15:20:03.120Z");

    // Escribe la hora actual. Devuelve false si todavia no hay hora NTP.
    static bool escribirAhora(char* destino, size_t capacidad);

    // Escribe una hora dada en segundos desde 1970 y milisegundos. Devuelve false si la hora es
    // anterior a 2026 (reloj sin sincronizar) o si el texto no entra en el destino.
    static bool escribir(int64_t segundosDesde1970, uint16_t milisegundos, char* destino,
                         size_t capacidad);
};
