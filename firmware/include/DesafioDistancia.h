#pragma once

#include "Desafio.h"
#include "IndicadorLeds.h"
#include "SensorDistancia.h"

class DesafioDistancia : public Desafio {
public:
    DesafioDistancia(SensorDistancia& sensor, IndicadorLeds& leds);

    void iniciar(const ParametrosJuego& parametros) override;
    void actualizar() override;
    void detener() override;
    ResultadoDesafio resultado() const override;
    const char* nombre() const override;

private:
    enum class Estado : uint8_t { Inactivo, BuscandoDistancia, DentroDelRango };

    SensorDistancia& sensor_;
    IndicadorLeds& leds_;
    int distanciaMinimaCm_ = 0;
    int distanciaMaximaCm_ = 0;
    unsigned long inicioDentroDelRangoMs_ = 0;
    unsigned long ultimaMuestraEnRangoMs_ = 0;
    Estado estado_ = Estado::Inactivo;
    ResultadoDesafio resultado_ = ResultadoDesafio::EnCurso;

    void comprobarDistancia(int distanciaCm);
    void mantenerDistancia(unsigned long ahoraMs);
};
