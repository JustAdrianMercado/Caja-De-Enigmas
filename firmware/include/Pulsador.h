#pragma once

#include <stdint.h>

class Pulsador {
public:
    explicit Pulsador(uint8_t pin);

    void iniciar();
    void actualizar();
    bool estaPulsado() const;
    bool estaLiberado() const;
    bool fuePulsado();

private:
    static constexpr unsigned long AntirreboteMs = 50UL;

    uint8_t pin_;
    bool ultimaLecturaPulsada_ = false;
    bool estaPulsado_ = false;
    bool hayPulsacionPendiente_ = false;
    unsigned long ultimoCambioLecturaMs_ = 0;
};
