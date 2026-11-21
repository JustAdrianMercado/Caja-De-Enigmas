#pragma once

#include <stdint.h>

enum class EstadoMedicionDistancia : uint8_t { SinMedicion, SinEco, FueraDeRango, Valida };

class SensorDistancia {
public:
    SensorDistancia(uint8_t pinTrig, uint8_t pinEcho);

    void iniciar();
    bool actualizar();
    EstadoMedicionDistancia estadoMedicion() const;
    uint16_t ultimaDistanciaValidaCm() const;

private:
    void medir();
    unsigned long medirEcoUs() const;

    uint8_t pinTrig_;
    uint8_t pinEcho_;
    unsigned long ultimaMedicionMs_ = 0;
    EstadoMedicionDistancia estadoMedicion_ = EstadoMedicionDistancia::SinMedicion;
    uint16_t ultimaDistanciaValidaCm_ = 0;
};
