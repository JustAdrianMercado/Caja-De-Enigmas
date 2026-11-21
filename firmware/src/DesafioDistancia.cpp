#include "DesafioDistancia.h"

#include <Arduino.h>

namespace {

constexpr unsigned long TiempoObjetivoMs = 2000UL;

}

DesafioDistancia::DesafioDistancia(SensorDistancia& sensor, IndicadorLeds& leds)
    : sensor_(sensor), leds_(leds) {}

void DesafioDistancia::iniciar(const ParametrosJuego& parametros) {
    distanciaMinimaCm_ = parametros.distanciaObjetivoCm - parametros.toleranciaCm;
    distanciaMaximaCm_ = parametros.distanciaObjetivoCm + parametros.toleranciaCm;
    estado_ = Estado::BuscandoDistancia;
    resultado_ = ResultadoDesafio::EnCurso;
    leds_.apagarTodos();
}

void DesafioDistancia::actualizar() {
    if (estado_ == Estado::Inactivo || !sensor_.actualizar()) {
        return;
    }
    if (sensor_.estadoMedicion() != EstadoMedicionDistancia::Valida) {
        estado_ = Estado::BuscandoDistancia;
        leds_.apagarTodos();
        return;
    }

    comprobarDistancia(sensor_.ultimaDistanciaValidaCm());
}

void DesafioDistancia::detener() {
    estado_ = Estado::Inactivo;
    leds_.apagarTodos();
}

ResultadoDesafio DesafioDistancia::resultado() const {
    return resultado_;
}

const char* DesafioDistancia::nombre() const {
    return "distancia";
}

void DesafioDistancia::comprobarDistancia(int distanciaCm) {
    if (distanciaCm < distanciaMinimaCm_) {
        estado_ = Estado::BuscandoDistancia;
        leds_.encender(ColorLed::Amarillo);
        return;
    }
    if (distanciaCm > distanciaMaximaCm_) {
        estado_ = Estado::BuscandoDistancia;
        leds_.encender(ColorLed::Rojo);
        return;
    }

    mantenerDistancia(millis());
}

void DesafioDistancia::mantenerDistancia(unsigned long ahoraMs) {
    if (estado_ == Estado::BuscandoDistancia) {
        estado_ = Estado::DentroDelRango;
        inicioDentroDelRangoMs_ = ahoraMs;
        leds_.encender(ColorLed::Verde);
        return;
    }

    if (ahoraMs - inicioDentroDelRangoMs_ >= TiempoObjetivoMs) {
        resultado_ = ResultadoDesafio::Superado;
        detener();
    }
}
