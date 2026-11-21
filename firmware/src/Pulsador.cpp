#include "Pulsador.h"

#include <Arduino.h>

Pulsador::Pulsador(uint8_t pin) : pin_(pin) {}

void Pulsador::iniciar() {
    pinMode(pin_, INPUT_PULLUP);
    ultimaLecturaPulsada_ = digitalRead(pin_) == LOW;
    estaPulsado_ = ultimaLecturaPulsada_;
    ultimoCambioLecturaMs_ = millis();
    hayPulsacionPendiente_ = false;
}

void Pulsador::actualizar() {
    const bool lecturaPulsada = digitalRead(pin_) == LOW;
    const unsigned long ahoraMs = millis();

    if (lecturaPulsada != ultimaLecturaPulsada_) {
        ultimaLecturaPulsada_ = lecturaPulsada;
        ultimoCambioLecturaMs_ = ahoraMs;
    }

    if (lecturaPulsada == estaPulsado_ ||
        ahoraMs - ultimoCambioLecturaMs_ < AntirreboteMs) {
        return;
    }

    estaPulsado_ = lecturaPulsada;
    if (estaPulsado_) {
        hayPulsacionPendiente_ = true;
    }
}

bool Pulsador::estaPulsado() const {
    return estaPulsado_;
}

bool Pulsador::estaLiberado() const {
    return !estaPulsado_ && !ultimaLecturaPulsada_;
}

bool Pulsador::fuePulsado() {
    const bool huboPulsacion = hayPulsacionPendiente_;
    hayPulsacionPendiente_ = false;
    return huboPulsacion;
}
