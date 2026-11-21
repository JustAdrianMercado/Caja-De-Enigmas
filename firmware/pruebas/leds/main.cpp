#include <Arduino.h>

#include "ConfiguracionHardware.h"
#include "IndicadorLeds.h"

namespace {

constexpr unsigned long VelocidadSerial = 115200;
constexpr uint8_t CantidadDestellos = 3;
constexpr unsigned long IntervaloParpadeoMs = 300UL;

IndicadorLeds indicador(PinLedRojo, PinLedAmarillo, PinLedVerde);

void mostrarAyuda() {
    Serial.println(F("r, a, v: encender rojo, amarillo o verde. R, A, V: parpadear."));
    Serial.println(F("x: apagar. ?: ayuda. Puedes enviar otro comando durante el parpadeo."));
}

void atenderComando(int comando) {
    switch (comando) {
        case 'r': indicador.encender(ColorLed::Rojo); break;
        case 'a': indicador.encender(ColorLed::Amarillo); break;
        case 'v': indicador.encender(ColorLed::Verde); break;
        case 'R': indicador.parpadear(ColorLed::Rojo, CantidadDestellos, IntervaloParpadeoMs); break;
        case 'A': indicador.parpadear(ColorLed::Amarillo, CantidadDestellos, IntervaloParpadeoMs); break;
        case 'V': indicador.parpadear(ColorLed::Verde, CantidadDestellos, IntervaloParpadeoMs); break;
        case 'x': indicador.apagarTodos(); break;
        case '?': mostrarAyuda(); break;
    }
}

}

void setup() {
    Serial.begin(VelocidadSerial);
    indicador.iniciar();
    mostrarAyuda();
}

void loop() {
    const bool estabaParpadeando = indicador.estaParpadeando();
    indicador.actualizar();
    if (estabaParpadeando && !indicador.estaParpadeando()) {
        Serial.println(F("Parpadeo terminado."));
    }

    if (Serial.available() > 0) {
        atenderComando(Serial.read());
    }
}
