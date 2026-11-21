#include <Arduino.h>

#include "ConfiguracionHardware.h"
#include "Pulsador.h"

namespace {

constexpr unsigned long VelocidadSerial = 115200;

Pulsador pulsadorRojo(PinPulsadorRojo);
Pulsador pulsadorAmarillo(PinPulsadorAmarillo);
Pulsador pulsadorVerde(PinPulsadorVerde);
Pulsador pulsadorInicio(PinPulsadorInicio);

void reportarPulsacion(Pulsador& pulsador, const __FlashStringHelper* nombre) {
    pulsador.actualizar();
    if (!pulsador.fuePulsado()) {
        return;
    }

    Serial.print(millis());
    Serial.print(F(" ms | Pulsado: "));
    Serial.println(nombre);
}

}

void setup() {
    Serial.begin(VelocidadSerial);
    pulsadorRojo.iniciar();
    pulsadorAmarillo.iniciar();
    pulsadorVerde.iniciar();
    pulsadorInicio.iniciar();
    Serial.println(F("Pulsa rojo, amarillo, verde o inicio. Debe aparecer una linea por pulsacion."));
}

void loop() {
    reportarPulsacion(pulsadorRojo, F("rojo"));
    reportarPulsacion(pulsadorAmarillo, F("amarillo"));
    reportarPulsacion(pulsadorVerde, F("verde"));
    reportarPulsacion(pulsadorInicio, F("inicio"));
}
