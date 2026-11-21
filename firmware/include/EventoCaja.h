#pragma once

#include <stdint.h>

#include "ParametrosJuego.h"
#include "TiposCaja.h"

// Rangos del contrato para los eventos (seccion 3.2). El ESP32 satura en estos valores para
// que la Lambda de almacenamiento no descarte un evento por una partida muy larga.
constexpr uint32_t DuracionMaximaInformadaMs = 3600000UL;  // 1 h.
constexpr uint8_t IntentosMaximosInformados = 99;

// Eventos que publica el ESP32 en caja-enigmas/<thing>/eventos. Las pistas las publica la
// Lambda de la skill, por eso no estan aca.
enum class TipoEvento : uint8_t {
    PartidaIniciada,
    DesafioSuperado,
    DesafioFallado,
    PartidaResuelta,
    Cerradura
};

/**
 * Un evento de la caja, todavia sin formato JSON (docs/contratos.md, seccion 3.2).
 *
 * Cada tipo usa solo algunos campos:
 * - PartidaIniciada: idPartida, origen y dificultad.
 * - DesafioSuperado: idPartida, desafio, duracionMs e intentos.
 * - DesafioFallado: idPartida y desafio.
 * - PartidaResuelta: idPartida y duracionMs (de toda la partida).
 * - Cerradura: cerradura y origen.
 */
struct EventoCaja {
    TipoEvento tipo = TipoEvento::PartidaIniciada;
    uint32_t idPartida = 0;
    const char* desafio = nullptr;  // nombre() del desafio: una cadena que nunca se libera.
    uint32_t duracionMs = 0;
    uint8_t intentos = 0;
    OrigenEvento origen = OrigenEvento::Edge;
    Dificultad dificultad = DificultadPorDefecto;
    EstadoCerradura cerradura = EstadoCerradura::Cerrada;
};

/** Recibe los eventos de la partida: main.cpp mueve el servo y los publica; los tests los anotan. */
class ReceptorEventos {
public:
    virtual ~ReceptorEventos() = default;
    virtual void recibirEvento(const EventoCaja& evento) = 0;
};
