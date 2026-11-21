#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Desafio.h"
#include "EventoCaja.h"
#include "ParametrosJuego.h"
#include "TiposCaja.h"

/**
 * Logica de control del Edge: la partida de tres desafios (docs/contratos.md, seccion 9.2).
 *
 * Recorre esperando -> memoria -> distancia -> luz -> resuelta, cuenta los intentos y la
 * duracion de cada desafio y avisa cada evento al receptor, incluidos los movimientos de la
 * cerradura que decide el juego: cerrar al iniciar y abrir al resolver.
 *
 * No usa Arduino: el tiempo llega como parametro, asi se prueba en la PC con desafios falsos
 * (firmware/test/test_partida_enigmas).
 */
class PartidaEnigmas {
public:
    // Los desafios y el receptor deben seguir existiendo durante toda la vida de la partida.
    PartidaEnigmas(Desafio& memoria, Desafio& distancia, Desafio& luz, ReceptorEventos& receptor);

    // Contadores guardados en la NVS, al arrancar. No inicia nada ni avisa eventos.
    void restaurarContadores(uint32_t idPartida, uint32_t solicitudPartida);

    // Modo, dificultad y calibracion vigentes. Los parametros nuevos se aplican en el proximo
    // desafio que empiece. Pasar a modo manual abandona la partida (seccion 9.1).
    void establecerConfiguracion(const ConfiguracionJuego& configuracion);

    // Pulsador de inicio: en modo juego empieza una partida nueva, aunque haya otra en curso.
    void pulsarInicio(uint32_t ahoraMs);

    // solicitudPartida del delta. Un valor distinto del ultimo se adopta y, en modo juego,
    // inicia una partida. El mismo valor repetido no hace nada (seccion 2).
    void recibirSolicitudPartida(uint32_t solicitudPartida, uint32_t ahoraMs);

    // Llamar en cada vuelta de loop(): actualiza el desafio en curso y avanza la partida.
    void actualizar(uint32_t ahoraMs);

    EstadoPartida estado() const;
    // nombre() del desafio en curso, o "ninguno".
    const char* desafioActual() const;
    uint32_t idPartida() const;
    uint32_t solicitudPartida() const;
    const ConfiguracionJuego& configuracion() const;

private:
    static constexpr size_t CantidadDesafios = 3;

    Desafio* desafios_[CantidadDesafios];  // En el orden en que se juegan.
    ReceptorEventos& receptor_;
    ConfiguracionJuego configuracion_;
    EstadoPartida estado_ = EstadoPartida::Esperando;
    size_t indiceDesafio_ = 0;
    uint8_t intentos_ = 0;
    uint32_t idPartida_ = 0;
    uint32_t solicitudPartida_ = 0;
    uint32_t inicioPartidaMs_ = 0;
    uint32_t inicioDesafioMs_ = 0;  // Primer intento del desafio en curso.

    void iniciarPartida(OrigenEvento origen, uint32_t ahoraMs);
    void abandonarPartida();
    void empezarDesafio(size_t indice, uint32_t ahoraMs);
    void atenderSuperado(uint32_t ahoraMs);
    void atenderFallo();
    void resolverPartida(uint32_t ahoraMs);

    Desafio& desafioEnCurso() const;
    EventoCaja eventoDeLaPartida(TipoEvento tipo) const;
    void avisarCerradura(EstadoCerradura cerradura);
};
