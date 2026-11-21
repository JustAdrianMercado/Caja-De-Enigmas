#pragma once

#include <stdint.h>

enum class ColorLed : uint8_t { Rojo, Amarillo, Verde };

class IndicadorLeds {
public:
    IndicadorLeds(uint8_t pinRojo, uint8_t pinAmarillo, uint8_t pinVerde);

    void iniciar();
    void actualizar();
    void encender(ColorLed color);
    void apagarTodos();
    void parpadear(ColorLed color, uint8_t cantidad, unsigned long intervaloMs);
    bool estaParpadeando() const;

private:
    uint8_t pines_[3];
    ColorLed colorParpadeo_ = ColorLed::Rojo;
    uint8_t destellosPendientes_ = 0;
    bool estaEncendido_ = false;
    unsigned long intervaloParpadeoMs_ = 0;
    unsigned long ultimoCambioMs_ = 0;
};
