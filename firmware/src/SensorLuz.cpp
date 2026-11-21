#include "SensorLuz.h"

#include <Arduino.h>

namespace {

constexpr uint8_t CantidadMuestras = 8;
constexpr int PorcentajeMaximo = 100;

}

SensorLuz::SensorLuz(uint8_t pin) : pin_(pin) {}

void SensorLuz::iniciar() {
    pinMode(pin_, INPUT);
    actualizar();
}

void SensorLuz::actualizar() {
    int sumaAdc = 0;
    for (uint8_t muestra = 0; muestra < CantidadMuestras; ++muestra) {
        sumaAdc += analogRead(pin_);
    }
    valorAdc_ = sumaAdc / CantidadMuestras;
}

bool SensorLuz::configurarCalibracion(int adcOscuridad, int adcLuzIntensa) {
    if (adcOscuridad < AdcMinimo || adcOscuridad > AdcMaximo ||
        adcLuzIntensa < AdcMinimo || adcLuzIntensa > AdcMaximo ||
        adcOscuridad == adcLuzIntensa) {
        return false;
    }

    adcOscuridad_ = adcOscuridad;
    adcLuzIntensa_ = adcLuzIntensa;
    return true;
}

int SensorLuz::valorAdc() const {
    return valorAdc_;
}

uint8_t SensorLuz::luzPorcentaje() const {
    const int porcentaje = PorcentajeMaximo * (valorAdc_ - adcOscuridad_) /
                           (adcLuzIntensa_ - adcOscuridad_);
    return static_cast<uint8_t>(constrain(porcentaje, 0, PorcentajeMaximo));
}

int SensorLuz::adcOscuridad() const {
    return adcOscuridad_;
}

int SensorLuz::adcLuzIntensa() const {
    return adcLuzIntensa_;
}
