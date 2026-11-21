#pragma once

#include <stdint.h>

class SensorLuz {
public:
    explicit SensorLuz(uint8_t pin);

    void iniciar();
    void actualizar();
    bool configurarCalibracion(int adcOscuridad, int adcLuzIntensa);
    int valorAdc() const;
    uint8_t luzPorcentaje() const;
    int adcOscuridad() const;
    int adcLuzIntensa() const;

private:
    static constexpr int AdcMinimo = 0;
    static constexpr int AdcMaximo = 4095;

    uint8_t pin_;
    int valorAdc_ = 0;
    int adcOscuridad_ = AdcMinimo;
    int adcLuzIntensa_ = AdcMaximo;
};
