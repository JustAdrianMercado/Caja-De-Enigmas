// Caja de los Enigmas - objeto inteligente.
// Sincroniza su estado con el Device Shadow de AWS IoT Core (contrato v2): aplica el modo, la
// dificultad y los parametros que llegan en el delta y los guarda en la NVS, mueve el servo de
// la cerradura por voz solo en modo manual y reporta el estado completo.
// Por Serial acepta comandos de prueba (escribir "ayuda").

#include <Arduino.h>
#include <string.h>

#include "AlmacenParametros.h"
#include "CerraduraServo.h"
#include "CodigoVinculacion.h"
#include "ConexionAwsIot.h"
#include "ConexionWifi.h"
#include "ConfiguracionHardware.h"
#include "ConsolaSerial.h"
#include "EstadoShadow.h"
#include "ParametrosJuego.h"

#if __has_include("ConfiguracionWifi.h")
#include "ConfiguracionWifi.h"
#else
#include "ConfiguracionWifi.ejemplo.h"
#endif

#if __has_include("ConfiguracionAws.h")
#include "ConfiguracionAws.h"
#else
#include "ConfiguracionAws.ejemplo.h"
#endif

namespace {

constexpr char VersionFirmware[] = "0.3.0";
constexpr unsigned long IntervaloReportePeriodicoMs = 60000UL;  // Respaldo (contratos, sec. 3).
constexpr size_t TamanoReporte = 512;  // El reporte v2 mas largo ocupa unos 370 bytes.

ConexionWifi conexionWifi(NombreRedWifi, ClaveRedWifi);
ConexionAwsIot conexionAws(conexionWifi, EndpointAwsIot, PuertoAwsIot, NombreThing,
                           CertificadoCaAmazon, CertificadoDispositivo, ClavePrivadaDispositivo);
CerraduraServo cerraduraServo(PinServoCerradura, AnguloCerraduraCerradaGrados,
                              AnguloCerraduraAbiertaGrados);
CodigoVinculacion codigoVinculacion;
EstadoShadow estadoShadow(codigoVinculacion.texto(), VersionFirmware);
AlmacenParametros almacenParametros;
ConsolaSerial consola;

ConfiguracionJuego configuracion;               // La vigente; se carga de la NVS al arrancar.
ConexionAwsIot::MensajeShadow mensajeRecibido;  // Global: 3 KB fuera de la pila de loop().
unsigned long ultimoReporteMs = 0;

// --- Actuador --------------------------------------------------------------------------------

void moverCerradura(EstadoCerradura estadoPedido) {
    if (estadoPedido == EstadoCerradura::Abierta) {
        cerraduraServo.abrir();
    } else {
        cerraduraServo.cerrar();
    }
    estadoShadow.establecerCerradura(estadoPedido);
    Serial.print(F("Cerradura: "));
    Serial.println(EstadoShadow::textoCerradura(estadoPedido));
}

// --- Configuracion del juego -----------------------------------------------------------------

void mostrarConfiguracion() {
    Serial.print(F("Configuracion: modo="));
    Serial.print(EstadoShadow::textoModo(configuracion.modo));
    Serial.print(F(" dificultad="));
    Serial.print(ParametrosJuego::textoDificultad(configuracion.dificultad));
    Serial.print(F(" distanciaObjetivoCm="));
    Serial.print(configuracion.distanciaObjetivoCm);
    Serial.print(F(" umbralLuzPorcentaje="));
    Serial.println(configuracion.umbralLuzPorcentaje);
}

void cargarConfiguracionGuardada() {
    configuracion = almacenParametros.cargarConfiguracion();
    estadoShadow.establecerConfiguracion(configuracion);
    estadoShadow.establecerPartida(EstadoPartida::Esperando, NombreSinDesafio,
                                   almacenParametros.cargarIdPartida(),
                                   almacenParametros.cargarSolicitudPartida());
    mostrarConfiguracion();
}

// --- Ordenes del Shadow ----------------------------------------------------------------------

void aplicarConfiguracion(const OrdenesShadow& ordenes) {
    const ConfiguracionJuego nueva = ordenes.aplicarA(configuracion);
    if (nueva == configuracion) {
        return;
    }
    configuracion = nueva;
    almacenParametros.guardarConfiguracion(configuracion);
    estadoShadow.establecerConfiguracion(configuracion);
    mostrarConfiguracion();
}

void aplicarOrdenCerradura(const OrdenesShadow& ordenes) {
    if (!ordenes.cerradura.esValida) {
        return;
    }
    if (!ordenes.pideMoverCerradura(configuracion)) {
        Serial.println(F("Orden: en modo juego la cerradura la mueve el juego; se ignora."));
        return;
    }
    Serial.print(F("Orden del Shadow: "));
    moverCerradura(ordenes.cerradura.valorPedido);
}

void aplicarOrdenes(const OrdenesShadow& ordenes) {
    if (!ordenes.hayAlgunaPresente()) {
        return;
    }
    if (ordenes.hayAlgunaInvalida()) {
        // Contrato: los valores invalidos no cambian nada; se reporta el estado real.
        Serial.println(F("Orden: hay valores invalidos en el delta; se ignoran."));
    }
    aplicarConfiguracion(ordenes);  // Primero: el modo nuevo decide si se obedece la cerradura.
    aplicarOrdenCerradura(ordenes);
    // Reportar aunque nada cambie: el delta indica que la nube tiene otros valores.
    estadoShadow.solicitarReporte();
}

void procesarMensaje(const ConexionAwsIot::MensajeShadow& mensaje) {
    OrdenesShadow ordenes;
    bool sePudoLeer = false;
    if (mensaje.tipo == ConexionAwsIot::TipoMensaje::Delta) {
        sePudoLeer = EstadoShadow::leerOrdenesDelta(mensaje.contenido, ordenes);
    } else {
        sePudoLeer = EstadoShadow::leerOrdenesDocumentoCompleto(mensaje.contenido, ordenes);
    }
    if (!sePudoLeer) {
        Serial.println(F("Shadow: mensaje con JSON invalido; se ignora."));
        return;
    }
    aplicarOrdenes(ordenes);
}

// --- Reportes --------------------------------------------------------------------------------

void publicarReporteSiHaceFalta() {
    if (!estadoShadow.hayCambiosSinReportar() || !conexionAws.estaConectada()) {
        return;
    }
    char documento[TamanoReporte];
    if (!estadoShadow.escribirReporte(documento, sizeof(documento))) {
        Serial.println(F("Shadow: el reporte no entra en el buffer."));
        return;
    }
    if (conexionAws.publicarActualizacion(documento)) {
        estadoShadow.confirmarReporte();
        ultimoReporteMs = millis();
        Serial.print(F("Shadow: reportado "));
        Serial.println(documento);
    }
}

void solicitarReportePeriodico() {
    // La resta sin signo sigue funcionando cuando millis() se desborda.
    if (millis() - ultimoReporteMs >= IntervaloReportePeriodicoMs) {
        estadoShadow.solicitarReporte();
    }
}

// --- Consola de prueba -----------------------------------------------------------------------

void mostrarAyuda() {
    Serial.println(F("Comandos: abrir, cerrar, estado, codigo, ayuda."));
    Serial.println(F("abrir y cerrar mueven el servo en cualquier modo, para probar el montaje."));
    Serial.println(F("En modo manual, el desired del Shadow manda: puede revertir abrir/cerrar."));
}

void mostrarEstado() {
    Serial.print(F("Estado: cerradura="));
    Serial.print(EstadoShadow::textoCerradura(estadoShadow.cerradura()));
    Serial.print(F(" aws="));
    Serial.println(conexionAws.estaConectada() ? F("conectado") : F("desconectado"));
    mostrarConfiguracion();
}

void ejecutarComando(const char* comando) {
    if (strcmp(comando, "abrir") == 0) {
        moverCerradura(EstadoCerradura::Abierta);
    } else if (strcmp(comando, "cerrar") == 0) {
        moverCerradura(EstadoCerradura::Cerrada);
    } else if (strcmp(comando, "estado") == 0) {
        mostrarEstado();
    } else if (strcmp(comando, "codigo") == 0) {
        Serial.print(F("Vinculacion: codigo de esta caja = "));
        Serial.println(codigoVinculacion.texto());
    } else {
        mostrarAyuda();
    }
}

void atenderConsola() {
    if (consola.leerComando()) {
        ejecutarComando(consola.comando());
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.print(F("Caja de los Enigmas - SIS-234 - firmware "));
    Serial.println(VersionFirmware);

    cerraduraServo.iniciar();  // Arranca cerrada.
    estadoShadow.establecerCerradura(EstadoCerradura::Cerrada);
    cargarConfiguracionGuardada();

    codigoVinculacion.iniciar();
    Serial.print(F("Vinculacion: codigo de esta caja = "));
    Serial.println(codigoVinculacion.texto());

    conexionWifi.iniciar();
    conexionAws.iniciar();
    mostrarAyuda();
}

void loop() {
    conexionWifi.actualizar();
    conexionAws.actualizar();

    if (conexionAws.seAcabaDeConectar()) {
        estadoShadow.solicitarReporte();
    }
    while (conexionAws.tomarMensaje(mensajeRecibido)) {
        procesarMensaje(mensajeRecibido);
    }
    atenderConsola();
    solicitarReportePeriodico();
    publicarReporteSiHaceFalta();
}
