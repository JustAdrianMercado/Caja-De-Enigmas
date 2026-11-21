#include "DesafioLuz.h"

#include <Arduino.h>

namespace {

constexpr unsigned long TiempoObjetivoMs = 1000UL;
constexpr unsigned long IntervaloMaximoEntreMuestrasMs = 200UL;

}

DesafioLuz::DesafioLuz(SensorLuz& sensor, IndicadorLeds& leds) : sensor_(sensor), leds_(leds) {}

void DesafioLuz::iniciar(const ParametrosJuego& parametros) {
    umbralLuzPorcentaje_ = parametros.umbralLuzPorcentaje;
    estado_ = Estado::BuscandoLuz;
    resultado_ = ResultadoDesafio::EnCurso;
    leds_.apagarTodos();
}

void DesafioLuz::actualizar() {
    if (estado_ == Estado::Inactivo) {
        return;
    }

    sensor_.actualizar();
    if (sensor_.luzPorcentaje() >= umbralLuzPorcentaje_) {
        mantenerLuz(millis());
        return;
    }

    if (estado_ == Estado::ManteniendoLuz) {
        estado_ = Estado::BuscandoLuz;
        leds_.apagarTodos();
    }
}

void DesafioLuz::detener() {
    estado_ = Estado::Inactivo;
    leds_.apagarTodos();
}

ResultadoDesafio DesafioLuz::resultado() const {
    return resultado_;
}

const char* DesafioLuz::nombre() const {
    return "luz";
}

void DesafioLuz::mantenerLuz(unsigned long ahoraMs) {
    const bool hayContinuidad = estado_ == Estado::ManteniendoLuz &&
                               ahoraMs - ultimaLuzSuficienteMs_ <= IntervaloMaximoEntreMuestrasMs;
    ultimaLuzSuficienteMs_ = ahoraMs;
    if (!hayContinuidad) {
        estado_ = Estado::ManteniendoLuz;
        inicioLuzSuficienteMs_ = ahoraMs;
        leds_.encender(ColorLed::Verde);
        return;
    }

    if (ahoraMs - inicioLuzSuficienteMs_ >= TiempoObjetivoMs) {
        resultado_ = ResultadoDesafio::Superado;
        detener();
    }
}
