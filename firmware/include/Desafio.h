#pragma once

#include <stdint.h>

#include "ParametrosJuego.h"

enum class ResultadoDesafio : uint8_t { EnCurso, Superado, Fallado };

/**
 * Interfaz comun de los tres desafios (docs/contratos.md, seccion 10).
 *
 * PartidaEnigmas los encadena sin saber que hardware usa cada uno. Reglas para implementarla:
 * - Ningun metodo bloquea loop(): nada de delay(). Los tiempos se miden con millis(), y
 *   pulseIn del HC-SR04 tiene un limite de unos 25 ms, con una medicion cada 100 ms como maximo.
 * - Cada desafio maneja sus propios LEDs.
 * - Superado y Fallado son finales: el resultado no cambia hasta el proximo iniciar().
 * - Despues de Fallado, PartidaEnigmas cuenta el intento y vuelve a llamar a iniciar().
 */
class Desafio {
public:
    virtual ~Desafio() = default;

    // Empieza de cero. Copia los parametros que necesita: si cambian por voz durante la
    // partida, se aplican recien en el proximo iniciar().
    virtual void iniciar(const ParametrosJuego& parametros) = 0;

    // Se llama en cada vuelta de loop() mientras el desafio esta activo.
    virtual void actualizar() = 0;

    // Se llama al abandonar la partida (paso a modo manual o reinicio): apaga sus LEDs y
    // deja de medir. Despues solo se vuelve a usar con iniciar().
    virtual void detener() = 0;

    // EnCurso antes de iniciar() y mientras el jugador no termina.
    virtual ResultadoDesafio resultado() const = 0;

    // "memoria", "distancia" o "luz", igual que en el Shadow y en los eventos.
    virtual const char* nombre() const = 0;
};
