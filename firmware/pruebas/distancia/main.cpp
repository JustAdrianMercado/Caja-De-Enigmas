#include <Arduino.h>

#include "ConfiguracionHardware.h"
#include "SensorDistancia.h"

namespace {

constexpr unsigned long VelocidadSerial = 115200;

SensorDistancia sensor(PinDistanciaTrig, PinDistanciaEcho);

void mostrarMedicion() {
    Serial.print(millis());
    Serial.print(F(" ms | "));
    switch (sensor.estadoMedicion()) {
        case EstadoMedicionDistancia::Valida:
            Serial.print(sensor.ultimaDistanciaValidaCm());
            Serial.println(F(" cm"));
            break;
        case EstadoMedicionDistancia::SinEco:
            Serial.println(F("Lectura invalida: sin eco."));
            break;
        case EstadoMedicionDistancia::FueraDeRango:
            Serial.println(F("Lectura invalida: fuera de 2 a 400 cm."));
            break;
        case EstadoMedicionDistancia::SinMedicion:
            Serial.println(F("Aun no hay medicion."));
            break;
    }
}

}

void setup() {
    Serial.begin(VelocidadSerial);
    sensor.iniciar();
    Serial.println(F("Diagnostico HC-SR04. Usa una regla para comparar 10, 20 y 30 cm."));
}

void loop() {
    if (sensor.actualizar()) {
        mostrarMedicion();
    }
}
