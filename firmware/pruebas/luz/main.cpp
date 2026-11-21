#include <Arduino.h>

#include "ConfiguracionHardware.h"
#include "SensorLuz.h"

namespace {

constexpr unsigned long VelocidadSerial = 115200;
constexpr unsigned long IntervaloReporteMs = 500UL;

SensorLuz sensor(PinLdr);
unsigned long ultimoReporteMs = 0;

void mostrarCalibracion() {
    Serial.print(F("Calibracion provisional (ADC): oscuridad = "));
    Serial.print(sensor.adcOscuridad());
    Serial.print(F(", luz intensa = "));
    Serial.println(sensor.adcLuzIntensa());
}

void mostrarMedicion() {
    Serial.print(F("ADC promedio: "));
    Serial.print(sensor.valorAdc());
    Serial.print(F(" | Luz: "));
    Serial.print(sensor.luzPorcentaje());
    Serial.println(F(" %"));
}

}

void setup() {
    Serial.begin(VelocidadSerial);
    sensor.iniciar();
    mostrarCalibracion();
    Serial.println(F("Compara oscuridad, luz ambiente y linterna."));
    mostrarMedicion();
    ultimoReporteMs = millis();
}

void loop() {
    const unsigned long ahoraMs = millis();
    if (ahoraMs - ultimoReporteMs < IntervaloReporteMs) {
        return;
    }

    ultimoReporteMs = ahoraMs;
    sensor.actualizar();
    mostrarMedicion();
}
