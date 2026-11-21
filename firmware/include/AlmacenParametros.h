#pragma once

#include <stdint.h>

#include "ParametrosJuego.h"

/**
 * Guarda en la NVS la configuracion del juego y los contadores de partida (docs/contratos.md,
 * seccion 2). Asi, despues de un reinicio sin red, la caja sigue configurada igual y no repite
 * un idPartida ni una solicitudPartida ya atendida.
 *
 * Si la NVS esta vacia o tiene valores invalidos, devuelve los valores de fabrica.
 */
class AlmacenParametros {
public:
    ConfiguracionJuego cargarConfiguracion() const;
    void guardarConfiguracion(const ConfiguracionJuego& configuracion) const;

    uint32_t cargarIdPartida() const;
    void guardarIdPartida(uint32_t idPartida) const;

    uint32_t cargarSolicitudPartida() const;
    void guardarSolicitudPartida(uint32_t solicitudPartida) const;

private:
    uint32_t cargarContador(const char* clave) const;
    void guardarContador(const char* clave, uint32_t valor) const;
};
