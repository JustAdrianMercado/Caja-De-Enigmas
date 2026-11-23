#pragma once

#include <stddef.h>
#include <stdint.h>

#include "EventoJson.h"

/**
 * Cola de eventos ya convertidos a JSON, pendientes de publicar (docs/contratos.md, seccion 3.3).
 *
 * Todos los eventos pasan por aca, haya conexion o no: asi se publican siempre en el orden en
 * que ocurrieron. Sin conexion se acumulan hasta 20; si se llena, se descarta el mas viejo y se
 * cuenta. No depende de Arduino: se prueba en la PC (firmware/test/test_cola_eventos).
 */
class ColaEventos {
public:
    static constexpr size_t Capacidad = 20;
    // Un evento que AWS rechaza estando conectados (p. ej., sin permiso en la politica) se
    // descarta al tercer intento, para que no trabe la cola ni reinicie la conexion sin fin.
    static constexpr uint8_t FallosAntesDeDescartar = 3;

    // Agrega al final; si la cola esta llena, descarta el mas viejo. Devuelve false, sin
    // agregar nada, si el JSON esta vacio o no entra en EventoJson::TamanoMaximo.
    bool agregar(const char* json);

    bool estaVacia() const;
    size_t cantidad() const;

    // El evento mas viejo, o nullptr si la cola esta vacia.
    const char* primero() const;
    // Llamar cuando el primero se publico con exito.
    void quitarPrimero();
    // Llamar cuando publicar el primero fallo estando conectados. Devuelve true si lo descarto.
    bool registrarFalloDelPrimero();

    // Eventos perdidos desde el arranque: por cola llena o por rechazos repetidos.
    uint32_t descartados() const;

private:
    char eventos_[Capacidad][EventoJson::TamanoMaximo] = {};
    size_t indicePrimero_ = 0;
    size_t cantidad_ = 0;
    uint8_t fallosDelPrimero_ = 0;
    uint32_t descartados_ = 0;

    void descartarPrimero();
};
