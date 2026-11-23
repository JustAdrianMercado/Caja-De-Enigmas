#pragma once

#include <stddef.h>

#include "EventoCaja.h"

/**
 * Arma el JSON de un evento para el tema caja-enigmas/<thing>/eventos (docs/contratos.md,
 * seccion 3.2). No depende de Arduino: se prueba en la PC (firmware/test/test_evento_json).
 *
 * Cada tipo lleva solo sus campos: la Lambda de almacenamiento valida todos los campos que
 * llegan, y un campo de mas con un valor fuera de rango haria descartar el evento entero.
 */
class EventoJson {
public:
    // El evento mas largo ocupa unos 150 bytes; este tamano deja margen.
    static constexpr size_t TamanoMaximo = 192;

    // tsObjeto es la hora del evento en ISO-8601 UTC con ms, o nullptr si no hay hora NTP:
    // en ese caso se omite. Devuelve false si el JSON no entra en el destino.
    static bool escribir(const EventoCaja& evento, const char* tsObjeto, char* destino,
                         size_t capacidad);

    static const char* textoTipo(TipoEvento tipo);
    static const char* textoOrigen(OrigenEvento origen);
};
