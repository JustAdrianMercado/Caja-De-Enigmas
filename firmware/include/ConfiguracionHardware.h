#pragma once

#include <stdint.h>

// Pines y calibracion del montaje (docs/contratos.md, seccion 8; docs/GUIA_MONTAJE.md).
// No contiene secretos: se versiona. Al calibrar el montaje real, cambiar solo este archivo.

constexpr uint8_t PinServoCerradura = 18;  // Senal del SG90; se alimenta con el VIN de 5 V del USB.
// Solo para el diagnostico de la P3 (firmware/pruebas/reed). El firmware de la P4 no usa el reed.
constexpr uint8_t PinReedTapa = 4;         // Reed a GND con INPUT_PULLUP: LOW = tapa cerrada.

constexpr uint8_t PinLedRojo = 25;
constexpr uint8_t PinLedAmarillo = 26;
constexpr uint8_t PinLedVerde = 27;

constexpr uint8_t PinPulsadorRojo = 13;
constexpr uint8_t PinPulsadorAmarillo = 14;
constexpr uint8_t PinPulsadorVerde = 16;
constexpr uint8_t PinPulsadorInicio = 17;

constexpr uint8_t PinDistanciaTrig = 23;
constexpr uint8_t PinDistanciaEcho = 19;
constexpr uint8_t PinLdr = 34;

// Dentro del rango seguro 45-135 de firmware/pruebas/servo. Probados en el montaje del 24/11/2026:
// el servo cerro y abrio en las 15 partidas. Si se acopla un pestillo real, recalibrar con esa prueba.
constexpr int AnguloCerraduraCerradaGrados = 45;
constexpr int AnguloCerraduraAbiertaGrados = 135;
