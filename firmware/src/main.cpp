// Caja de los Enigmas - objeto inteligente (contrato v2).
// - Juego: la partida de tres desafios (PartidaEnigmas) corre en el ESP32 y funciona sin red.
// - Shadow: aplica el modo, la dificultad, los parametros y el inicio por voz que llegan en el
//   delta, los guarda en la NVS y reporta el estado completo con las lecturas de los sensores.
// - Eventos: cada evento del juego y de la cerradura pasa por una cola y se publica en
//   caja-enigmas/<thing>/eventos; sin conexion se guardan hasta 20.
// Por Serial acepta comandos de prueba (escribir "ayuda").

#include <Arduino.h>
#include <string.h>

#include "AlmacenParametros.h"
#include "CerraduraServo.h"
#include "CodigoVinculacion.h"
#include "ColaEventos.h"
#include "ConexionAwsIot.h"
#include "ConexionWifi.h"
#include "ConfiguracionHardware.h"
#include "ConsolaSerial.h"
#include "DesafioDistancia.h"
#include "DesafioLuz.h"
#include "DesafioMemoria.h"
#include "EstadoShadow.h"
#include "EventoJson.h"
#include "IndicadorLeds.h"
#include "ParametrosJuego.h"
#include "PartidaEnigmas.h"
#include "Pulsador.h"
#include "RelojUtc.h"
#include "SensorDistancia.h"
#include "SensorLuz.h"

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

constexpr char VersionFirmware[] = "0.4.0";
constexpr unsigned long IntervaloReportePeriodicoMs = 60000UL;  // Respaldo (contratos, sec. 3).
constexpr size_t TamanoReporte = 512;  // El reporte v2 mas largo ocupa unos 370 bytes.

// --- Conexion, Shadow y persistencia ---------------------------------------------------------

ConexionWifi conexionWifi(NombreRedWifi, ClaveRedWifi);
ConexionAwsIot conexionAws(conexionWifi, EndpointAwsIot, PuertoAwsIot, NombreThing,
                           CertificadoCaAmazon, CertificadoDispositivo, ClavePrivadaDispositivo);
CodigoVinculacion codigoVinculacion;
EstadoShadow estadoShadow(codigoVinculacion.texto(), VersionFirmware);
AlmacenParametros almacenParametros;
ColaEventos colaEventos;
ConsolaSerial consola;

ConexionAwsIot::MensajeShadow mensajeRecibido;  // Global: 3 KB fuera de la pila de loop().
unsigned long ultimoReporteMs = 0;

// --- Hardware (docs/contratos.md, seccion 8) y desafios ---------------------------------------

CerraduraServo cerraduraServo(PinServoCerradura, AnguloCerraduraCerradaGrados,
                              AnguloCerraduraAbiertaGrados);
IndicadorLeds leds(PinLedRojo, PinLedAmarillo, PinLedVerde);
Pulsador pulsadorRojo(PinPulsadorRojo);
Pulsador pulsadorAmarillo(PinPulsadorAmarillo);
Pulsador pulsadorVerde(PinPulsadorVerde);
Pulsador pulsadorInicio(PinPulsadorInicio);
SensorDistancia sensorDistancia(PinDistanciaTrig, PinDistanciaEcho);
SensorLuz sensorLuz(PinLdr);

DesafioMemoria desafioMemoria(leds, pulsadorRojo, pulsadorAmarillo, pulsadorVerde);
DesafioDistancia desafioDistancia(sensorDistancia, leds);
DesafioLuz desafioLuz(sensorLuz, leds);

// --- Eventos ---------------------------------------------------------------------------------

// Se convierte a JSON en el momento, para que tsObjeto sea la hora del evento y no la de
// publicacion. Sin hora NTP (arranque sin red) se publica sin tsObjeto.
void encolarEvento(const EventoCaja& evento) {
    char tsObjeto[RelojUtc::TamanoTexto];
    const bool hayHora = RelojUtc::escribirAhora(tsObjeto, sizeof(tsObjeto));
    char json[EventoJson::TamanoMaximo];
    if (!EventoJson::escribir(evento, hayHora ? tsObjeto : nullptr, json, sizeof(json))) {
        Serial.println(F("Eventos: el evento no entra en el buffer; se pierde."));
        return;
    }
    colaEventos.agregar(json);  // Con la cola llena se pierde el mas viejo, y se cuenta.
    Serial.print(F("Evento: "));
    Serial.println(json);
}

// Uno por vuelta de loop(): cada publicacion espera el PUBACK, y vaciar la cola entera de una
// vez frenaria los desafios al reconectar.
void publicarEventoPendiente() {
    if (colaEventos.estaVacia() || !conexionAws.estaConectada()) {
        return;
    }
    if (conexionAws.publicarEvento(colaEventos.primero())) {
        colaEventos.quitarPrimero();
        Serial.print(F("Eventos: publicado; quedan "));
        Serial.println(colaEventos.cantidad());
        return;
    }
    if (colaEventos.registrarFalloDelPrimero()) {
        Serial.println(F("Eventos: AWS rechazo 3 veces el mismo evento; se descarta."));
        Serial.println(F("Revisar el permiso del tema de eventos en la politica de IoT."));
    }
}

// --- Cerradura -------------------------------------------------------------------------------

void moverServo(EstadoCerradura estadoPedido) {
    if (estadoPedido == EstadoCerradura::Abierta) {
        cerraduraServo.abrir();
    } else {
        cerraduraServo.cerrar();
    }
    estadoShadow.establecerCerradura(estadoPedido);
    // La marca de millis() sirve para medir el tiempo de respuesta del Edge por Serial.
    Serial.print(F("Cerradura: "));
    Serial.print(EstadoShadow::textoCerradura(estadoPedido));
    Serial.print(F(" a los "));
    Serial.print(millis());
    Serial.println(F(" ms"));
}

void moverCerraduraPorVoz(EstadoCerradura estadoPedido) {
    moverServo(estadoPedido);
    EventoCaja evento;
    evento.tipo = TipoEvento::Cerradura;
    evento.cerradura = estadoPedido;
    evento.origen = OrigenEvento::Voz;  // Solo la skill escribe desired.cerradura.
    encolarEvento(evento);
}

/** Atiende los eventos de la partida: mueve el servo cuando lo pide el juego y los encola. */
class ReceptorCaja : public ReceptorEventos {
public:
    void recibirEvento(const EventoCaja& evento) override {
        if (evento.tipo == TipoEvento::Cerradura) {
            moverServo(evento.cerradura);
        }
        if (evento.tipo == TipoEvento::PartidaIniciada) {
            almacenParametros.guardarIdPartida(evento.idPartida);  // No repetirlo tras un reinicio.
        }
        encolarEvento(evento);
    }
};

ReceptorCaja receptorCaja;
PartidaEnigmas partida(desafioMemoria, desafioDistancia, desafioLuz, receptorCaja);

// --- Configuracion y partida -------------------------------------------------------------------

void mostrarConfiguracion() {
    const ConfiguracionJuego& configuracion = partida.configuracion();
    Serial.print(F("Configuracion: modo="));
    Serial.print(EstadoShadow::textoModo(configuracion.modo));
    Serial.print(F(" dificultad="));
    Serial.print(ParametrosJuego::textoDificultad(configuracion.dificultad));
    Serial.print(F(" distanciaObjetivoCm="));
    Serial.print(configuracion.distanciaObjetivoCm);
    Serial.print(F(" umbralLuzPorcentaje="));
    Serial.println(configuracion.umbralLuzPorcentaje);
}

void sincronizarPartidaConShadow() {
    estadoShadow.establecerPartida(partida.estado(), partida.desafioActual(), partida.idPartida(),
                                   partida.solicitudPartida());
}

void cargarEstadoGuardado() {
    partida.establecerConfiguracion(almacenParametros.cargarConfiguracion());
    partida.restaurarContadores(almacenParametros.cargarIdPartida(),
                                almacenParametros.cargarSolicitudPartida());
    estadoShadow.establecerConfiguracion(partida.configuracion());
    sincronizarPartidaConShadow();
    mostrarConfiguracion();
}

void atenderPulsadorInicio() {
    pulsadorInicio.actualizar();
    if (pulsadorInicio.fuePulsado()) {
        partida.pulsarInicio(millis());  // En modo manual no hace nada.
    }
}

// --- Ordenes del Shadow ----------------------------------------------------------------------

void aplicarConfiguracion(const OrdenesShadow& ordenes) {
    const ConfiguracionJuego nueva = ordenes.aplicarA(partida.configuracion());
    if (nueva == partida.configuracion()) {
        return;
    }
    partida.establecerConfiguracion(nueva);  // Pasar a manual abandona la partida.
    almacenParametros.guardarConfiguracion(nueva);
    estadoShadow.establecerConfiguracion(nueva);
    mostrarConfiguracion();
}

void aplicarOrdenCerradura(const OrdenesShadow& ordenes) {
    if (!ordenes.cerradura.esValida) {
        return;
    }
    if (!ordenes.pideMoverCerradura(partida.configuracion())) {
        Serial.println(F("Orden: en modo juego la cerradura la mueve el juego; se ignora."));
        return;
    }
    Serial.print(F("Orden del Shadow. "));
    moverCerraduraPorVoz(ordenes.cerradura.valorPedido);
}

void aplicarSolicitudPartida(const OrdenesShadow& ordenes) {
    if (!ordenes.solicitudPartida.esValida) {
        return;
    }
    const uint32_t solicitudAnterior = partida.solicitudPartida();
    partida.recibirSolicitudPartida(ordenes.solicitudPartida.valorPedido, millis());
    if (partida.solicitudPartida() != solicitudAnterior) {
        almacenParametros.guardarSolicitudPartida(partida.solicitudPartida());
    }
}

void aplicarOrdenes(const OrdenesShadow& ordenes) {
    if (!ordenes.hayAlgunaPresente()) {
        return;
    }
    if (ordenes.hayAlgunaInvalida()) {
        // Contrato: los valores invalidos no cambian nada; se reporta el estado real.
        Serial.println(F("Orden: hay valores invalidos en el delta; se ignoran."));
    }
    // Primero la configuracion: el modo nuevo decide si se obedece la cerradura y si la
    // solicitud inicia una partida.
    aplicarConfiguracion(ordenes);
    aplicarOrdenCerradura(ordenes);
    aplicarSolicitudPartida(ordenes);
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

// Durante el desafio de distancia mide el propio desafio: llamar aca a actualizar() le quitaria
// la medicion nueva. Fuera de ese desafio, la medicion se toma justo antes de reportar.
void tomarLecturas() {
    if (strcmp(partida.desafioActual(), desafioDistancia.nombre()) != 0) {
        sensorDistancia.actualizar();
    }
    sensorLuz.actualizar();
    const bool hayDistancia =
        sensorDistancia.estadoMedicion() == EstadoMedicionDistancia::Valida;
    const int32_t distanciaCm =
        hayDistancia ? static_cast<int32_t>(sensorDistancia.ultimaDistanciaValidaCm()) : SinLectura;
    estadoShadow.actualizarLecturas(distanciaCm, sensorLuz.luzPorcentaje());
}

void publicarReporteSiHaceFalta() {
    if (!estadoShadow.hayCambiosSinReportar() || !conexionAws.estaConectada()) {
        return;
    }
    tomarLecturas();
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
    Serial.println(F("Comandos: iniciar, abrir, cerrar, estado, codigo, ayuda."));
    Serial.println(F("iniciar equivale al pulsador de inicio (solo en modo juego)."));
    Serial.println(F("abrir y cerrar mueven el servo en cualquier modo, para probar el montaje."));
}

void mostrarEstado() {
    Serial.print(F("Estado: cerradura="));
    Serial.print(EstadoShadow::textoCerradura(estadoShadow.cerradura()));
    Serial.print(F(" aws="));
    Serial.println(conexionAws.estaConectada() ? F("conectado") : F("desconectado"));
    Serial.print(F("Partida: "));
    Serial.print(EstadoShadow::textoEstadoPartida(partida.estado()));
    Serial.print(F(" desafio="));
    Serial.print(partida.desafioActual());
    Serial.print(F(" idPartida="));
    Serial.println(partida.idPartida());
    Serial.print(F("Eventos: en cola="));
    Serial.print(colaEventos.cantidad());
    Serial.print(F(" descartados="));
    Serial.println(colaEventos.descartados());
    mostrarConfiguracion();
}

void ejecutarComando(const char* comando) {
    if (strcmp(comando, "iniciar") == 0) {
        partida.pulsarInicio(millis());
    } else if (strcmp(comando, "abrir") == 0) {
        moverServo(EstadoCerradura::Abierta);
    } else if (strcmp(comando, "cerrar") == 0) {
        moverServo(EstadoCerradura::Cerrada);
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

void iniciarHardwareDelJuego() {
    cerraduraServo.iniciar();  // Arranca cerrada.
    estadoShadow.establecerCerradura(EstadoCerradura::Cerrada);
    leds.iniciar();
    pulsadorRojo.iniciar();
    pulsadorAmarillo.iniciar();
    pulsadorVerde.iniciar();
    pulsadorInicio.iniciar();
    sensorDistancia.iniciar();
    sensorLuz.iniciar();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.print(F("Caja de los Enigmas - SIS-234 - firmware "));
    Serial.println(VersionFirmware);

    iniciarHardwareDelJuego();
    cargarEstadoGuardado();

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

    // El juego corre en cada vuelta, haya red o no.
    atenderPulsadorInicio();
    partida.actualizar(millis());
    leds.actualizar();
    sincronizarPartidaConShadow();

    atenderConsola();
    solicitarReportePeriodico();
    publicarReporteSiHaceFalta();
    publicarEventoPendiente();
}
