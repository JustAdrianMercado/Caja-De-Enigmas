#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ParametrosJuego.h"
#include "TiposCaja.h"

// Valor de una lectura cuando no hay sensor o la medicion no sirvio: no se reporta.
constexpr int32_t SinLectura = -1;

/** Un atributo que pide un mensaje del Shadow. */
template <typename Valor>
struct Orden {
    bool estaPresente = false;  // El mensaje incluye el atributo.
    bool esValida = false;      // Su valor esta en la lista o en el rango del contrato.
    Valor valorPedido{};        // Solo tiene sentido si esValida.
};

/** Lo que pide un mensaje del Shadow. Cada atributo se lee y se valida por separado. */
struct OrdenesShadow {
    Orden<EstadoCerradura> cerradura;
    Orden<ModoCaja> modo;
    Orden<Dificultad> dificultad;
    Orden<uint8_t> distanciaObjetivoCm;
    Orden<uint8_t> umbralLuzPorcentaje;
    Orden<uint32_t> solicitudPartida;

    bool hayAlgunaPresente() const;
    bool hayAlgunaInvalida() const;

    // La configuracion con las ordenes validas aplicadas; las invalidas no cambian nada.
    ConfiguracionJuego aplicarA(const ConfiguracionJuego& actual) const;

    // true si hay que mover la cerradura: la orden es valida y el modo la acepta (seccion 9.1).
    // Recibe la configuracion ya pasada por aplicarA(), para que cuente un cambio de modo que
    // venga en el mismo mensaje.
    bool pideMoverCerradura(const ConfiguracionJuego& configuracionNueva) const;
};

/**
 * Estado del objeto que se sincroniza con el Device Shadow (docs/contratos.md, seccion 2).
 *
 * Guarda lo que el ESP32 reporta, lee las ordenes que llegan en el delta y arma el documento
 * "reported". No depende de Arduino, por eso se prueba en la PC (firmware/test).
 */
class EstadoShadow {
public:
    // Las cadenas deben seguir existiendo durante toda la vida del objeto.
    EstadoShadow(const char* codigoVinculacion, const char* versionFirmware);

    EstadoCerradura cerradura() const;
    void establecerCerradura(EstadoCerradura nuevoEstado);
    void establecerConfiguracion(const ConfiguracionJuego& configuracion);
    // desafioActual es el nombre() del desafio o "ninguno"; la cadena debe seguir existiendo.
    void establecerPartida(EstadoPartida estadoPartida, const char* desafioActual,
                           uint32_t idPartida, uint32_t solicitudPartida);

    // Las lecturas no piden un reporte propio: viajan en el periodico y en el de cada cambio de
    // desafio (seccion 3.3). SinLectura, o un valor fuera del rango del contrato, no se reporta.
    void actualizarLecturas(int32_t distanciaCm, int32_t luzPorcentaje);

    // Pide reportar aunque nada haya cambiado: al reconectar, cada 60 s o tras una orden.
    void solicitarReporte();
    bool hayCambiosSinReportar() const;

    // Escribe {"state":{"reported":{...}}}. Devuelve false si el documento no entra en el destino.
    bool escribirReporte(char* destino, size_t capacidad) const;

    // Llamar solo cuando el reporte se publico con exito.
    void confirmarReporte();

    // Mensaje de update/delta: {"state":{"modo":"manual", ...}, ...}.
    static bool leerOrdenesDelta(const char* json, OrdenesShadow& ordenes);

    // Mensaje de get/accepted: las ordenes pendientes estan en {"state":{"delta":{...}}}.
    static bool leerOrdenesDocumentoCompleto(const char* json, OrdenesShadow& ordenes);

    static const char* textoCerradura(EstadoCerradura estado);
    static const char* textoModo(ModoCaja modo);
    static const char* textoEstadoPartida(EstadoPartida estado);

private:
    const char* codigoVinculacion_;
    const char* versionFirmware_;
    EstadoCerradura cerradura_ = EstadoCerradura::Cerrada;
    ConfiguracionJuego configuracion_;
    EstadoPartida estadoPartida_ = EstadoPartida::Esperando;
    const char* desafioActual_ = NombreSinDesafio;
    uint32_t idPartida_ = 0;
    uint32_t solicitudPartida_ = 0;
    int32_t distanciaCm_ = SinLectura;
    int32_t luzPorcentaje_ = SinLectura;
    bool hayCambiosSinReportar_ = true;  // El primer reporte siempre se envia.
};
