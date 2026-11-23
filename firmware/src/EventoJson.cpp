#include "EventoJson.h"

#include <ArduinoJson.h>

#include "EstadoShadow.h"
#include "ParametrosJuego.h"

namespace {

void agregarInicio(const EventoCaja& evento, JsonObject json) {
    json["idPartida"] = evento.idPartida;
    json["origen"] = EventoJson::textoOrigen(evento.origen);
    json["dificultad"] = ParametrosJuego::textoDificultad(evento.dificultad);
}

void agregarSuperado(const EventoCaja& evento, JsonObject json) {
    json["idPartida"] = evento.idPartida;
    json["desafio"] = evento.desafio;
    json["duracionMs"] = evento.duracionMs;
    json["intentos"] = evento.intentos;
}

void agregarFallado(const EventoCaja& evento, JsonObject json) {
    json["idPartida"] = evento.idPartida;
    json["desafio"] = evento.desafio;
}

void agregarResuelta(const EventoCaja& evento, JsonObject json) {
    json["idPartida"] = evento.idPartida;
    json["duracionMs"] = evento.duracionMs;
}

void agregarCerradura(const EventoCaja& evento, JsonObject json) {
    json["cerradura"] = EstadoShadow::textoCerradura(evento.cerradura);
    json["origen"] = EventoJson::textoOrigen(evento.origen);
}

void agregarCamposDelTipo(const EventoCaja& evento, JsonObject json) {
    switch (evento.tipo) {
        case TipoEvento::PartidaIniciada:
            agregarInicio(evento, json);
            break;
        case TipoEvento::DesafioSuperado:
            agregarSuperado(evento, json);
            break;
        case TipoEvento::DesafioFallado:
            agregarFallado(evento, json);
            break;
        case TipoEvento::PartidaResuelta:
            agregarResuelta(evento, json);
            break;
        case TipoEvento::Cerradura:
            agregarCerradura(evento, json);
            break;
    }
}

}  // namespace

bool EventoJson::escribir(const EventoCaja& evento, const char* tsObjeto, char* destino,
                          size_t capacidad) {
    if (destino == nullptr || capacidad == 0) {
        return false;
    }
    JsonDocument documento;
    JsonObject json = documento.to<JsonObject>();
    json["tipo"] = textoTipo(evento.tipo);
    agregarCamposDelTipo(evento, json);
    if (tsObjeto != nullptr) {
        json["tsObjeto"] = tsObjeto;
    }

    // measureJson no cuenta el terminador: hace falta un byte mas.
    if (measureJson(documento) >= capacidad) {
        return false;
    }
    serializeJson(documento, destino, capacidad);
    return true;
}

const char* EventoJson::textoTipo(TipoEvento tipo) {
    switch (tipo) {
        case TipoEvento::DesafioSuperado:
            return "desafio_superado";
        case TipoEvento::DesafioFallado:
            return "desafio_fallado";
        case TipoEvento::PartidaResuelta:
            return "partida_resuelta";
        case TipoEvento::Cerradura:
            return "cerradura";
        case TipoEvento::PartidaIniciada:
        default:
            return "partida_iniciada";
    }
}

const char* EventoJson::textoOrigen(OrigenEvento origen) {
    switch (origen) {
        case OrigenEvento::Voz:
            return "voz";
        case OrigenEvento::Nube:
            return "nube";
        case OrigenEvento::Edge:
        default:
            return "edge";
    }
}
