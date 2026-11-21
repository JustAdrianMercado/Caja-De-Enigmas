#pragma once

#include <stdint.h>

// Valores que comparten el Shadow, los eventos y la partida (docs/contratos.md, secciones 2,
// 3.2 y 9). Estan aparte para que esos modulos no dependan unos de otros.

enum class EstadoCerradura : uint8_t { Cerrada, Abierta };

// Contrato, seccion 9.1: en modo juego la cerradura la mueve solo la partida; en manual, la voz.
// La NVS guarda el numero de cada valor: no cambiar el orden, solo agregar al final.
enum class ModoCaja : uint8_t { Juego, Manual };

enum class EstadoPartida : uint8_t { Esperando, EnCurso, Resuelta };

// Quien provoco un evento: el juego en el ESP32, una orden de voz o una regla de la Nube.
enum class OrigenEvento : uint8_t { Edge, Voz, Nube };

// Valor de desafioActual en el Shadow cuando no hay un desafio en curso.
constexpr char NombreSinDesafio[] = "ninguno";
