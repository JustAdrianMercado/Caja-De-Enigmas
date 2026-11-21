#pragma once

#include "Desafio.h"
#include "IndicadorLeds.h"
#include "Pulsador.h"

class DesafioMemoria : public Desafio {
public:
    DesafioMemoria(IndicadorLeds& leds, Pulsador& rojo, Pulsador& amarillo, Pulsador& verde);

    void iniciar(const ParametrosJuego& parametros) override;
    void actualizar() override;
    void detener() override;
    ResultadoDesafio resultado() const override;
    const char* nombre() const override;

private:
    enum class Estado : uint8_t {
        Inactivo, MostrandoColor, PausaEntreColores, EsperandoLiberacion, RecibiendoRespuesta
    };

    static constexpr uint8_t CantidadColores = 3;
    static constexpr uint8_t LongitudMaximaSecuencia = 6;

    IndicadorLeds& leds_;
    Pulsador* pulsadores_[CantidadColores];
    ColorLed secuencia_[LongitudMaximaSecuencia];
    uint8_t longitudSecuencia_ = 0;
    uint8_t posicion_ = 0;
    unsigned long inicioEstadoMs_ = 0;
    Estado estado_ = Estado::Inactivo;
    ResultadoDesafio resultado_ = ResultadoDesafio::EnCurso;

    void mostrarColor(unsigned long ahoraMs);
    void continuarPresentacion(unsigned long ahoraMs);
    void actualizarRespuesta();
    void esperarLiberacion();
    void comprobarRespuesta();
    void terminar(ResultadoDesafio resultado);
};
