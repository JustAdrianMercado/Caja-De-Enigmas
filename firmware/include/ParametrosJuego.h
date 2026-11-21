#pragma once

#include <stdint.h>

#include "TiposCaja.h"

// Parametros y configuracion del juego, tabla de dificultades y rangos validos
// (docs/contratos.md, seccion 2). Logica pura, sin Arduino: se prueba en la PC
// (firmware/test/test_parametros_juego).

// La NVS guarda el numero de cada valor: no cambiar el orden, solo agregar al final.
enum class Dificultad : uint8_t { Facil, Normal, Dificil };

// Rangos que acepta el contrato para los parametros que se cambian por voz.
constexpr uint8_t DistanciaObjetivoMinimaCm = 10;
constexpr uint8_t DistanciaObjetivoMaximaCm = 40;
constexpr uint8_t UmbralLuzMinimoPorcentaje = 10;
constexpr uint8_t UmbralLuzMaximoPorcentaje = 95;

// Valores con los que arranca una caja nueva. Los de calibracion son PROVISIONALES: se
// ajustan con el montaje real y, al cambiarlos, se actualiza tambien el contrato.
constexpr Dificultad DificultadPorDefecto = Dificultad::Normal;
constexpr uint8_t DistanciaObjetivoPorDefectoCm = 20;
constexpr uint8_t UmbralLuzPorDefectoPorcentaje = 70;

/**
 * Lo que necesita un desafio para empezar (docs/contratos.md, seccion 10).
 *
 * La dificultad fija longitudSecuencia y toleranciaCm; distanciaObjetivoCm y
 * umbralLuzPorcentaje son de calibracion y se cambian por separado.
 */
struct ParametrosJuego {
    uint8_t longitudSecuencia;
    uint8_t toleranciaCm;
    uint8_t distanciaObjetivoCm;
    uint8_t umbralLuzPorcentaje;

    // Combina la tabla de la dificultad con la calibracion. No valida: usar los es...Valido().
    static ParametrosJuego armar(Dificultad dificultad, uint8_t distanciaObjetivoCm,
                                 uint8_t umbralLuzPorcentaje);
    static ParametrosJuego porDefecto();

    // Tabla de dificultades: facil = 3 y 6 cm, normal = 4 y 4 cm, dificil = 6 y 2 cm.
    static uint8_t longitudSecuenciaDe(Dificultad dificultad);
    static uint8_t toleranciaCmDe(Dificultad dificultad);

    // Reciben int32_t para detectar los negativos y los mayores que 255 que llegan del Shadow.
    static bool esDistanciaObjetivoValida(int32_t distanciaCm);
    static bool esUmbralLuzValido(int32_t porcentaje);

    // "facil", "normal" o "dificil", igual que en el Shadow y en los eventos.
    static const char* textoDificultad(Dificultad dificultad);
    // Devuelve false y no cambia la dificultad si el texto no es uno de esos tres.
    static bool leerDificultad(const char* texto, Dificultad& dificultad);
};

/**
 * Lo que se cambia por voz y se guarda en la NVS: el modo, la dificultad y la calibracion.
 * De aca salen los ParametrosJuego con los que empieza cada desafio.
 */
struct ConfiguracionJuego {
    ModoCaja modo = ModoCaja::Juego;
    Dificultad dificultad = DificultadPorDefecto;
    uint8_t distanciaObjetivoCm = DistanciaObjetivoPorDefectoCm;
    uint8_t umbralLuzPorcentaje = UmbralLuzPorDefectoPorcentaje;

    ParametrosJuego parametros() const;

    // Contrato, seccion 9.1: las ordenes de voz sobre la cerradura solo valen en modo manual.
    bool aceptaOrdenesDeCerradura() const;

    // Arma la configuracion con lo leido de la NVS. Cada valor invalido (NVS vacia o con
    // datos de otra version) se reemplaza por el de fabrica, sin tocar los demas.
    static ConfiguracionJuego desdeValoresGuardados(uint8_t modo, uint8_t dificultad,
                                                    uint8_t distanciaObjetivoCm,
                                                    uint8_t umbralLuzPorcentaje);
};

bool operator==(const ConfiguracionJuego& una, const ConfiguracionJuego& otra);
bool operator!=(const ConfiguracionJuego& una, const ConfiguracionJuego& otra);
