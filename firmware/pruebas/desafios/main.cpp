#include <Arduino.h>
#include <string.h>

#include "CerraduraServo.h"
#include "ConfiguracionHardware.h"
#include "DesafioDistancia.h"
#include "DesafioLuz.h"
#include "DesafioMemoria.h"
#include "IndicadorLeds.h"
#include "PartidaEnigmas.h"
#include "Pulsador.h"
#include "SensorDistancia.h"
#include "SensorLuz.h"

namespace {

constexpr unsigned long VelocidadSerial = 115200;

class ReceptorDiagnostico : public ReceptorEventos {
public:
    explicit ReceptorDiagnostico(CerraduraServo& cerradura) : cerradura_(cerradura) {}

    void recibirEvento(const EventoCaja& evento) override {
        switch (evento.tipo) {
            case TipoEvento::Cerradura:
                moverCerradura(evento.cerradura);
                break;
            case TipoEvento::PartidaIniciada:
                Serial.print(F("Partida iniciada. Numero: "));
                Serial.println(evento.idPartida);
                break;
            case TipoEvento::DesafioSuperado:
                Serial.print(F("Desafio superado: "));
                Serial.println(evento.desafio);
                break;
            case TipoEvento::DesafioFallado:
                Serial.print(F("Desafio fallado: "));
                Serial.print(evento.desafio);
                Serial.println(F(". Reintentando."));
                break;
            case TipoEvento::PartidaResuelta:
                Serial.println(F("Partida completada. Pulsa inicio para jugar otra vez."));
                break;
        }
    }

private:
    CerraduraServo& cerradura_;

    void moverCerradura(EstadoCerradura estado) {
        if (estado == EstadoCerradura::Abierta) {
            cerradura_.abrir();
            Serial.println(F("Evento de apertura: orden enviada al servo."));
            return;
        }

        cerradura_.cerrar();
        Serial.println(F("Evento de cierre: orden enviada al servo."));
    }
};

Pulsador pulsadorRojo(PinPulsadorRojo);
Pulsador pulsadorAmarillo(PinPulsadorAmarillo);
Pulsador pulsadorVerde(PinPulsadorVerde);
Pulsador pulsadorInicio(PinPulsadorInicio);
IndicadorLeds leds(PinLedRojo, PinLedAmarillo, PinLedVerde);
SensorDistancia sensorDistancia(PinDistanciaTrig, PinDistanciaEcho);
SensorLuz sensorLuz(PinLdr);
CerraduraServo cerradura(PinServoCerradura, AnguloCerraduraCerradaGrados,
                         AnguloCerraduraAbiertaGrados);

DesafioMemoria memoria(leds, pulsadorRojo, pulsadorAmarillo, pulsadorVerde);
DesafioDistancia distancia(sensorDistancia, leds);
DesafioLuz luz(sensorLuz, leds);
ReceptorDiagnostico receptor(cerradura);
PartidaEnigmas partida(memoria, distancia, luz, receptor);

void mostrarConfiguracion(const ConfiguracionJuego& configuracion) {
    const ParametrosJuego parametros = configuracion.parametros();
    Serial.printf("Dificultad %s. Memoria: %d colores. Distancia: %d +/- %d cm. Luz: %d %%.\n",
                  ParametrosJuego::textoDificultad(configuracion.dificultad),
                  parametros.longitudSecuencia, parametros.distanciaObjetivoCm,
                  parametros.toleranciaCm, parametros.umbralLuzPorcentaje);
}

void mostrarDesafioSiCambio(const char* desafioAnterior, uint32_t idPartidaAnterior) {
    if (partida.estado() != EstadoPartida::EnCurso) {
        return;
    }

    const char* desafioActivo = partida.desafioActual();
    if (idPartidaAnterior != partida.idPartida() || strcmp(desafioAnterior, desafioActivo) != 0) {
        Serial.print(F("Desafio activo: "));
        Serial.println(desafioActivo);
    }
}

}

void setup() {
    Serial.begin(VelocidadSerial);
    pulsadorRojo.iniciar();
    pulsadorAmarillo.iniciar();
    pulsadorVerde.iniciar();
    pulsadorInicio.iniciar();
    leds.iniciar();
    sensorDistancia.iniciar();
    sensorLuz.iniciar();
    cerradura.iniciar();

    const ConfiguracionJuego configuracion;
    partida.establecerConfiguracion(configuracion);
    mostrarConfiguracion(configuracion);
    Serial.println(F("Diagnostico preparado. Esperando pulsador de inicio."));
}

void loop() {
    const char* desafioAnterior = partida.desafioActual();
    const uint32_t idPartidaAnterior = partida.idPartida();

    pulsadorInicio.actualizar();
    if (pulsadorInicio.fuePulsado()) {
        partida.pulsarInicio(millis());
    }

    partida.actualizar(millis());
    leds.actualizar();
    mostrarDesafioSiCambio(desafioAnterior, idPartidaAnterior);
}
