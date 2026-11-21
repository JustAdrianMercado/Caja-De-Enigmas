#include "PartidaEnigmas.h"

namespace {

// La resta sin signo sigue funcionando cuando millis() se desborda.
uint32_t duracionInformadaMs(uint32_t desdeMs, uint32_t hastaMs) {
    const uint32_t duracionMs = hastaMs - desdeMs;
    return duracionMs > DuracionMaximaInformadaMs ? DuracionMaximaInformadaMs : duracionMs;
}

}  // namespace

PartidaEnigmas::PartidaEnigmas(Desafio& memoria, Desafio& distancia, Desafio& luz,
                               ReceptorEventos& receptor)
    : desafios_{&memoria, &distancia, &luz}, receptor_(receptor) {}

void PartidaEnigmas::restaurarContadores(uint32_t idPartida, uint32_t solicitudPartida) {
    idPartida_ = idPartida;
    solicitudPartida_ = solicitudPartida;
}

void PartidaEnigmas::establecerConfiguracion(const ConfiguracionJuego& configuracion) {
    configuracion_ = configuracion;
    if (configuracion_.modo == ModoCaja::Manual) {
        abandonarPartida();
    }
}

void PartidaEnigmas::pulsarInicio(uint32_t ahoraMs) {
    if (configuracion_.modo != ModoCaja::Juego) {
        return;  // En modo manual el pulsador no hace nada.
    }
    iniciarPartida(OrigenEvento::Edge, ahoraMs);
}

void PartidaEnigmas::recibirSolicitudPartida(uint32_t solicitudPartida, uint32_t ahoraMs) {
    if (solicitudPartida == solicitudPartida_) {
        return;  // Ya atendida: llega repetida al reconectar o en el get del Shadow.
    }
    solicitudPartida_ = solicitudPartida;  // Se adopta tambien en manual, para limpiar el delta.
    if (configuracion_.modo == ModoCaja::Juego) {
        iniciarPartida(OrigenEvento::Voz, ahoraMs);
    }
}

void PartidaEnigmas::actualizar(uint32_t ahoraMs) {
    if (estado_ != EstadoPartida::EnCurso) {
        return;
    }
    Desafio& desafio = desafioEnCurso();
    desafio.actualizar();
    switch (desafio.resultado()) {
        case ResultadoDesafio::Superado:
            atenderSuperado(ahoraMs);
            break;
        case ResultadoDesafio::Fallado:
            atenderFallo();
            break;
        case ResultadoDesafio::EnCurso:
            break;
    }
}

EstadoPartida PartidaEnigmas::estado() const {
    return estado_;
}

const char* PartidaEnigmas::desafioActual() const {
    if (estado_ != EstadoPartida::EnCurso) {
        return NombreSinDesafio;
    }
    return desafioEnCurso().nombre();
}

uint32_t PartidaEnigmas::idPartida() const {
    return idPartida_;
}

uint32_t PartidaEnigmas::solicitudPartida() const {
    return solicitudPartida_;
}

const ConfiguracionJuego& PartidaEnigmas::configuracion() const {
    return configuracion_;
}

void PartidaEnigmas::iniciarPartida(OrigenEvento origen, uint32_t ahoraMs) {
    if (estado_ == EstadoPartida::EnCurso) {
        desafioEnCurso().detener();  // Iniciar en plena partida empieza una nueva.
    }
    ++idPartida_;
    estado_ = EstadoPartida::EnCurso;
    inicioPartidaMs_ = ahoraMs;
    empezarDesafio(0, ahoraMs);

    // Se avisa con la partida ya armada, para que el receptor vea el estado nuevo.
    avisarCerradura(EstadoCerradura::Cerrada);
    EventoCaja inicio = eventoDeLaPartida(TipoEvento::PartidaIniciada);
    inicio.origen = origen;
    inicio.dificultad = configuracion_.dificultad;
    receptor_.recibirEvento(inicio);
}

void PartidaEnigmas::abandonarPartida() {
    if (estado_ == EstadoPartida::EnCurso) {
        desafioEnCurso().detener();
    }
    estado_ = EstadoPartida::Esperando;
}

void PartidaEnigmas::empezarDesafio(size_t indice, uint32_t ahoraMs) {
    indiceDesafio_ = indice;
    intentos_ = 1;
    inicioDesafioMs_ = ahoraMs;
    desafioEnCurso().iniciar(configuracion_.parametros());
}

void PartidaEnigmas::atenderSuperado(uint32_t ahoraMs) {
    EventoCaja superado = eventoDeLaPartida(TipoEvento::DesafioSuperado);
    superado.desafio = desafioEnCurso().nombre();
    superado.duracionMs = duracionInformadaMs(inicioDesafioMs_, ahoraMs);
    superado.intentos = intentos_;
    receptor_.recibirEvento(superado);

    const size_t siguiente = indiceDesafio_ + 1;
    if (siguiente < CantidadDesafios) {
        empezarDesafio(siguiente, ahoraMs);
        return;
    }
    resolverPartida(ahoraMs);
}

void PartidaEnigmas::atenderFallo() {
    EventoCaja fallado = eventoDeLaPartida(TipoEvento::DesafioFallado);
    fallado.desafio = desafioEnCurso().nombre();
    receptor_.recibirEvento(fallado);

    if (intentos_ < IntentosMaximosInformados) {
        ++intentos_;
    }
    // Se reintenta el mismo desafio, con los parametros vigentes ahora.
    desafioEnCurso().iniciar(configuracion_.parametros());
}

void PartidaEnigmas::resolverPartida(uint32_t ahoraMs) {
    estado_ = EstadoPartida::Resuelta;
    avisarCerradura(EstadoCerradura::Abierta);
    EventoCaja resuelta = eventoDeLaPartida(TipoEvento::PartidaResuelta);
    resuelta.duracionMs = duracionInformadaMs(inicioPartidaMs_, ahoraMs);
    receptor_.recibirEvento(resuelta);
}

Desafio& PartidaEnigmas::desafioEnCurso() const {
    return *desafios_[indiceDesafio_];
}

EventoCaja PartidaEnigmas::eventoDeLaPartida(TipoEvento tipo) const {
    EventoCaja evento;
    evento.tipo = tipo;
    evento.idPartida = idPartida_;
    return evento;
}

void PartidaEnigmas::avisarCerradura(EstadoCerradura cerradura) {
    EventoCaja evento;
    evento.tipo = TipoEvento::Cerradura;
    evento.cerradura = cerradura;
    evento.origen = OrigenEvento::Edge;
    receptor_.recibirEvento(evento);
}
