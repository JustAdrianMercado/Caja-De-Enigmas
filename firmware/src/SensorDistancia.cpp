#include "SensorDistancia.h"

#include <Arduino.h>
#include <math.h>

namespace {

constexpr unsigned long IntervaloMedicionMs = 100UL;
constexpr unsigned long LimiteEsperaEcoUs = 25000UL;
constexpr unsigned int PulsoBajoPrevioUs = 2;
constexpr unsigned int PulsoDisparoUs = 10;
constexpr float VelocidadSonidoCmPorUs = 0.034f;
constexpr float DistanciaMinimaCm = 2.0f;
constexpr float DistanciaMaximaCm = 400.0f;

}

SensorDistancia::SensorDistancia(uint8_t pinTrig, uint8_t pinEcho)
    : pinTrig_(pinTrig), pinEcho_(pinEcho) {}

void SensorDistancia::iniciar() {
    pinMode(pinTrig_, OUTPUT);
    digitalWrite(pinTrig_, LOW);
    pinMode(pinEcho_, INPUT);
    ultimaMedicionMs_ = millis() - IntervaloMedicionMs;
    estadoMedicion_ = EstadoMedicionDistancia::SinMedicion;
    ultimaDistanciaValidaCm_ = 0;
}

bool SensorDistancia::actualizar() {
    const unsigned long ahoraMs = millis();
    if (ahoraMs - ultimaMedicionMs_ < IntervaloMedicionMs) {
        return false;
    }

    ultimaMedicionMs_ = ahoraMs;
    medir();
    return true;
}

EstadoMedicionDistancia SensorDistancia::estadoMedicion() const {
    return estadoMedicion_;
}

uint16_t SensorDistancia::ultimaDistanciaValidaCm() const {
    return ultimaDistanciaValidaCm_;
}

void SensorDistancia::medir() {
    const unsigned long duracionEcoUs = medirEcoUs();
    if (duracionEcoUs == 0) {
        estadoMedicion_ = EstadoMedicionDistancia::SinEco;
        return;
    }

    const float distanciaCm = duracionEcoUs * VelocidadSonidoCmPorUs / 2;
    if (distanciaCm < DistanciaMinimaCm || distanciaCm > DistanciaMaximaCm) {
        estadoMedicion_ = EstadoMedicionDistancia::FueraDeRango;
        return;
    }

    ultimaDistanciaValidaCm_ = static_cast<uint16_t>(lroundf(distanciaCm));
    estadoMedicion_ = EstadoMedicionDistancia::Valida;
}

unsigned long SensorDistancia::medirEcoUs() const {
    digitalWrite(pinTrig_, LOW);
    delayMicroseconds(PulsoBajoPrevioUs);
    digitalWrite(pinTrig_, HIGH);
    delayMicroseconds(PulsoDisparoUs);
    digitalWrite(pinTrig_, LOW);
    return pulseIn(pinEcho_, HIGH, LimiteEsperaEcoUs);
}
