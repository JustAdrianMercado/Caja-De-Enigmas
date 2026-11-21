#pragma once

#include "Desafio.h"
#include "IndicadorLeds.h"
#include "SensorLuz.h"

class DesafioLuz : public Desafio {
public:
    DesafioLuz(SensorLuz& sensor, IndicadorLeds& leds);

    void iniciar(const ParametrosJuego& parametros) override;
    void actualizar() override;
    void detener() override;
    ResultadoDesafio resultado() const override;
    const char* nombre() const override;

private:
    enum class Estado : uint8_t { Inactivo, BuscandoLuz, ManteniendoLuz };

    SensorLuz& sensor_;
    IndicadorLeds& leds_;
    uint8_t umbralLuzPorcentaje_ = 0;
    unsigned long inicioLuzSuficienteMs_ = 0;
    unsigned long ultimaLuzSuficienteMs_ = 0;
    Estado estado_ = Estado::Inactivo;
    ResultadoDesafio resultado_ = ResultadoDesafio::EnCurso;

    void mantenerLuz(unsigned long ahoraMs);
};
