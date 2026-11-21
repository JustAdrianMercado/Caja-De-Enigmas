#include "IndicadorLeds.h"

#include <Arduino.h>

IndicadorLeds::IndicadorLeds(uint8_t pinRojo, uint8_t pinAmarillo, uint8_t pinVerde)
    : pines_{pinRojo, pinAmarillo, pinVerde} {}

void IndicadorLeds::iniciar() {
    for (const uint8_t pin : pines_) {
        pinMode(pin, OUTPUT);
    }
    apagarTodos();
}

void IndicadorLeds::actualizar() {
    if (!estaParpadeando()) {
        return;
    }

    const unsigned long ahoraMs = millis();
    if (ahoraMs - ultimoCambioMs_ < intervaloParpadeoMs_) {
        return;
    }

    ultimoCambioMs_ = ahoraMs;
    estaEncendido_ = !estaEncendido_;
    digitalWrite(pines_[static_cast<uint8_t>(colorParpadeo_)], estaEncendido_ ? HIGH : LOW);
    if (!estaEncendido_) {
        --destellosPendientes_;
    }
}

void IndicadorLeds::encender(ColorLed color) {
    apagarTodos();
    digitalWrite(pines_[static_cast<uint8_t>(color)], HIGH);
}

void IndicadorLeds::apagarTodos() {
    destellosPendientes_ = 0;
    estaEncendido_ = false;
    for (const uint8_t pin : pines_) {
        digitalWrite(pin, LOW);
    }
}

void IndicadorLeds::parpadear(ColorLed color, uint8_t cantidad, unsigned long intervaloMs) {
    if (cantidad == 0 || intervaloMs == 0) {
        apagarTodos();
        return;
    }

    encender(color);
    colorParpadeo_ = color;
    destellosPendientes_ = cantidad;
    intervaloParpadeoMs_ = intervaloMs;
    ultimoCambioMs_ = millis();
    estaEncendido_ = true;
}

bool IndicadorLeds::estaParpadeando() const {
    return destellosPendientes_ > 0;
}
