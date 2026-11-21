#include "DesafioMemoria.h"

#include <Arduino.h>
#include <esp_random.h>

namespace {

constexpr unsigned long DuracionColorMs = 500UL;
constexpr unsigned long DuracionPausaMs = 250UL;

}

DesafioMemoria::DesafioMemoria(IndicadorLeds& leds, Pulsador& rojo, Pulsador& amarillo,
                             Pulsador& verde)
    : leds_(leds), pulsadores_{&rojo, &amarillo, &verde} {}

void DesafioMemoria::iniciar(const ParametrosJuego& parametros) {
    longitudSecuencia_ = parametros.longitudSecuencia;
    if (longitudSecuencia_ == 0 || longitudSecuencia_ > LongitudMaximaSecuencia) {
        longitudSecuencia_ = ParametrosJuego::longitudSecuenciaDe(DificultadPorDefecto);
    }
    for (uint8_t posicion = 0; posicion < longitudSecuencia_; ++posicion) {
        secuencia_[posicion] = static_cast<ColorLed>(esp_random() % CantidadColores);
    }

    posicion_ = 0;
    resultado_ = ResultadoDesafio::EnCurso;
    mostrarColor(millis());
}

void DesafioMemoria::actualizar() {
    const unsigned long ahoraMs = millis();
    switch (estado_) {
        case Estado::MostrandoColor:
            if (ahoraMs - inicioEstadoMs_ >= DuracionColorMs) {
                leds_.apagarTodos();
                estado_ = Estado::PausaEntreColores;
                inicioEstadoMs_ = ahoraMs;
            }
            break;
        case Estado::PausaEntreColores:
            if (ahoraMs - inicioEstadoMs_ >= DuracionPausaMs) {
                continuarPresentacion(ahoraMs);
            }
            break;
        case Estado::EsperandoLiberacion:
        case Estado::RecibiendoRespuesta:
            actualizarRespuesta();
            break;
        case Estado::Inactivo:
            break;
    }
}

void DesafioMemoria::detener() {
    estado_ = Estado::Inactivo;
    leds_.apagarTodos();
}

ResultadoDesafio DesafioMemoria::resultado() const {
    return resultado_;
}

const char* DesafioMemoria::nombre() const {
    return "memoria";
}

void DesafioMemoria::mostrarColor(unsigned long ahoraMs) {
    leds_.encender(secuencia_[posicion_]);
    estado_ = Estado::MostrandoColor;
    inicioEstadoMs_ = ahoraMs;
}

void DesafioMemoria::continuarPresentacion(unsigned long ahoraMs) {
    ++posicion_;
    if (posicion_ < longitudSecuencia_) {
        mostrarColor(ahoraMs);
        return;
    }

    posicion_ = 0;
    for (Pulsador* pulsador : pulsadores_) {
        pulsador->iniciar();
    }
    estado_ = Estado::EsperandoLiberacion;
}

void DesafioMemoria::actualizarRespuesta() {
    for (Pulsador* pulsador : pulsadores_) {
        pulsador->actualizar();
    }

    if (estado_ == Estado::EsperandoLiberacion) {
        esperarLiberacion();
        return;
    }
    comprobarRespuesta();
}

void DesafioMemoria::esperarLiberacion() {
    bool hayPulsadorPresionado = false;
    for (Pulsador* pulsador : pulsadores_) {
        pulsador->fuePulsado();
        hayPulsadorPresionado = hayPulsadorPresionado || pulsador->estaPulsado();
    }
    if (!hayPulsadorPresionado) {
        estado_ = Estado::RecibiendoRespuesta;
    }
}

void DesafioMemoria::comprobarRespuesta() {
    bool huboPulsacion = false;
    for (uint8_t indice = 0; indice < CantidadColores; ++indice) {
        if (!pulsadores_[indice]->fuePulsado()) {
            continue;
        }
        if (huboPulsacion || static_cast<ColorLed>(indice) != secuencia_[posicion_]) {
            terminar(ResultadoDesafio::Fallado);
            return;
        }
        huboPulsacion = true;
    }

    if (!huboPulsacion) {
        return;
    }

    ++posicion_;
    if (posicion_ == longitudSecuencia_) {
        terminar(ResultadoDesafio::Superado);
    }
}

void DesafioMemoria::terminar(ResultadoDesafio resultado) {
    resultado_ = resultado;
    detener();
}
