#include "EstadoShadow.h"

#include <ArduinoJson.h>
#include <string.h>

namespace {

// Nombres de los atributos del Shadow (docs/contratos.md, seccion 2).
constexpr char ClaveCerradura[] = "cerradura";
constexpr char ClaveModo[] = "modo";
constexpr char ClaveDificultad[] = "dificultad";
constexpr char ClaveDistanciaObjetivo[] = "distanciaObjetivoCm";
constexpr char ClaveUmbralLuz[] = "umbralLuzPorcentaje";
constexpr char ClaveSolicitudPartida[] = "solicitudPartida";

constexpr char TextoAbierta[] = "abierta";
constexpr char TextoCerrada[] = "cerrada";
constexpr char TextoJuego[] = "juego";
constexpr char TextoManual[] = "manual";
constexpr char TextoEsperando[] = "esperando";
constexpr char TextoEnCurso[] = "en_curso";
constexpr char TextoResuelta[] = "resuelta";

// Rangos de las lecturas en el contrato. Fuera de ellos la lectura no se reporta.
constexpr int32_t DistanciaMinimaReportableCm = 2;
constexpr int32_t DistanciaMaximaReportableCm = 400;
constexpr int32_t LuzMinimaPorcentaje = 0;
constexpr int32_t LuzMaximaPorcentaje = 100;

// --- Lectores del delta: cada uno acepta solo los valores validos del contrato --------------

bool leerCerradura(JsonVariantConst valor, EstadoCerradura& estado) {
    const char* texto = valor.as<const char*>();  // nullptr si el valor no es texto.
    if (texto == nullptr) {
        return false;
    }
    if (strcmp(texto, TextoAbierta) == 0) {
        estado = EstadoCerradura::Abierta;
        return true;
    }
    if (strcmp(texto, TextoCerrada) == 0) {
        estado = EstadoCerradura::Cerrada;
        return true;
    }
    return false;
}

bool leerModo(JsonVariantConst valor, ModoCaja& modo) {
    const char* texto = valor.as<const char*>();
    if (texto == nullptr) {
        return false;
    }
    if (strcmp(texto, TextoJuego) == 0) {
        modo = ModoCaja::Juego;
        return true;
    }
    if (strcmp(texto, TextoManual) == 0) {
        modo = ModoCaja::Manual;
        return true;
    }
    return false;
}

bool leerDificultad(JsonVariantConst valor, Dificultad& dificultad) {
    return ParametrosJuego::leerDificultad(valor.as<const char*>(), dificultad);
}

// is<int32_t>() rechaza el texto, los decimales y los numeros que no entran en 32 bits.
bool leerDistanciaObjetivo(JsonVariantConst valor, uint8_t& distanciaCm) {
    if (!valor.is<int32_t>()) {
        return false;
    }
    const int32_t pedido = valor.as<int32_t>();
    if (!ParametrosJuego::esDistanciaObjetivoValida(pedido)) {
        return false;
    }
    distanciaCm = static_cast<uint8_t>(pedido);
    return true;
}

bool leerUmbralLuz(JsonVariantConst valor, uint8_t& porcentaje) {
    if (!valor.is<int32_t>()) {
        return false;
    }
    const int32_t pedido = valor.as<int32_t>();
    if (!ParametrosJuego::esUmbralLuzValido(pedido)) {
        return false;
    }
    porcentaje = static_cast<uint8_t>(pedido);
    return true;
}

// is<uint32_t>() rechaza ademas los negativos.
bool leerSolicitudPartida(JsonVariantConst valor, uint32_t& solicitud) {
    if (!valor.is<uint32_t>()) {
        return false;
    }
    solicitud = valor.as<uint32_t>();
    return true;
}

template <typename Valor>
Orden<Valor> interpretar(JsonVariantConst valorRecibido,
                         bool (*leerValor)(JsonVariantConst, Valor&)) {
    Orden<Valor> orden;
    if (valorRecibido.isNull()) {
        return orden;  // El mensaje no habla de este atributo.
    }
    orden.estaPresente = true;
    orden.esValida = leerValor(valorRecibido, orden.valorPedido);
    return orden;
}

OrdenesShadow interpretarOrdenes(JsonObjectConst atributos) {
    OrdenesShadow ordenes;
    ordenes.cerradura = interpretar(atributos[ClaveCerradura], leerCerradura);
    ordenes.modo = interpretar(atributos[ClaveModo], leerModo);
    ordenes.dificultad = interpretar(atributos[ClaveDificultad], leerDificultad);
    ordenes.distanciaObjetivoCm =
        interpretar(atributos[ClaveDistanciaObjetivo], leerDistanciaObjetivo);
    ordenes.umbralLuzPorcentaje = interpretar(atributos[ClaveUmbralLuz], leerUmbralLuz);
    ordenes.solicitudPartida =
        interpretar(atributos[ClaveSolicitudPartida], leerSolicitudPartida);
    return ordenes;
}

// El filtro deja solo los atributos que se leen: descarta los metadatos y los desconocidos,
// y asi ahorra memoria al leer el documento completo.
void agregarAtributosAlFiltro(JsonObject filtro) {
    filtro[ClaveCerradura] = true;
    filtro[ClaveModo] = true;
    filtro[ClaveDificultad] = true;
    filtro[ClaveDistanciaObjetivo] = true;
    filtro[ClaveUmbralLuz] = true;
    filtro[ClaveSolicitudPartida] = true;
}

bool deserializarConFiltro(const char* json, const JsonDocument& filtro,
                           JsonDocument& documento) {
    if (json == nullptr) {
        return false;
    }
    const DeserializationError error =
        deserializeJson(documento, json, DeserializationOption::Filter(filtro));
    return !error;
}

template <typename Valor>
bool esInvalida(const Orden<Valor>& orden) {
    return orden.estaPresente && !orden.esValida;
}

bool esDistanciaReportable(int32_t distanciaCm) {
    return distanciaCm >= DistanciaMinimaReportableCm &&
           distanciaCm <= DistanciaMaximaReportableCm;
}

bool esLuzReportable(int32_t luzPorcentaje) {
    return luzPorcentaje >= LuzMinimaPorcentaje && luzPorcentaje <= LuzMaximaPorcentaje;
}

}  // namespace

// --- OrdenesShadow ----------------------------------------------------------------------------

bool OrdenesShadow::hayAlgunaPresente() const {
    return cerradura.estaPresente || modo.estaPresente || dificultad.estaPresente ||
           distanciaObjetivoCm.estaPresente || umbralLuzPorcentaje.estaPresente ||
           solicitudPartida.estaPresente;
}

bool OrdenesShadow::hayAlgunaInvalida() const {
    return esInvalida(cerradura) || esInvalida(modo) || esInvalida(dificultad) ||
           esInvalida(distanciaObjetivoCm) || esInvalida(umbralLuzPorcentaje) ||
           esInvalida(solicitudPartida);
}

ConfiguracionJuego OrdenesShadow::aplicarA(const ConfiguracionJuego& actual) const {
    ConfiguracionJuego nueva = actual;
    if (modo.esValida) {
        nueva.modo = modo.valorPedido;
    }
    if (dificultad.esValida) {
        nueva.dificultad = dificultad.valorPedido;
    }
    if (distanciaObjetivoCm.esValida) {
        nueva.distanciaObjetivoCm = distanciaObjetivoCm.valorPedido;
    }
    if (umbralLuzPorcentaje.esValida) {
        nueva.umbralLuzPorcentaje = umbralLuzPorcentaje.valorPedido;
    }
    return nueva;
}

bool OrdenesShadow::pideMoverCerradura(const ConfiguracionJuego& configuracionNueva) const {
    return cerradura.esValida && configuracionNueva.aceptaOrdenesDeCerradura();
}

// --- EstadoShadow -----------------------------------------------------------------------------

EstadoShadow::EstadoShadow(const char* codigoVinculacion, const char* versionFirmware)
    : codigoVinculacion_(codigoVinculacion), versionFirmware_(versionFirmware) {}

EstadoCerradura EstadoShadow::cerradura() const {
    return cerradura_;
}

void EstadoShadow::establecerCerradura(EstadoCerradura nuevoEstado) {
    if (nuevoEstado != cerradura_) {
        cerradura_ = nuevoEstado;
        hayCambiosSinReportar_ = true;
    }
}

void EstadoShadow::establecerConfiguracion(const ConfiguracionJuego& configuracion) {
    if (configuracion != configuracion_) {
        configuracion_ = configuracion;
        hayCambiosSinReportar_ = true;
    }
}

void EstadoShadow::establecerPartida(EstadoPartida estadoPartida, const char* desafioActual,
                                     uint32_t idPartida, uint32_t solicitudPartida) {
    const char* desafio = desafioActual != nullptr ? desafioActual : NombreSinDesafio;
    const bool huboCambio = estadoPartida != estadoPartida_ ||
                            strcmp(desafio, desafioActual_) != 0 || idPartida != idPartida_ ||
                            solicitudPartida != solicitudPartida_;
    if (!huboCambio) {
        return;
    }
    estadoPartida_ = estadoPartida;
    desafioActual_ = desafio;
    idPartida_ = idPartida;
    solicitudPartida_ = solicitudPartida;
    hayCambiosSinReportar_ = true;
}

void EstadoShadow::actualizarLecturas(int32_t distanciaCm, int32_t luzPorcentaje) {
    distanciaCm_ = distanciaCm;
    luzPorcentaje_ = luzPorcentaje;
}

void EstadoShadow::solicitarReporte() {
    hayCambiosSinReportar_ = true;
}

bool EstadoShadow::hayCambiosSinReportar() const {
    return hayCambiosSinReportar_;
}

void EstadoShadow::confirmarReporte() {
    hayCambiosSinReportar_ = false;
}

bool EstadoShadow::escribirReporte(char* destino, size_t capacidad) const {
    if (destino == nullptr || capacidad == 0) {
        return false;
    }
    const ParametrosJuego parametros = configuracion_.parametros();

    // En el orden de la tabla del contrato, para que sea facil compararlo.
    JsonDocument documento;
    JsonObject reportado = documento["state"]["reported"].to<JsonObject>();
    reportado[ClaveCerradura] = textoCerradura(cerradura_);
    reportado[ClaveModo] = textoModo(configuracion_.modo);
    reportado[ClaveDificultad] = ParametrosJuego::textoDificultad(configuracion_.dificultad);
    reportado[ClaveDistanciaObjetivo] = configuracion_.distanciaObjetivoCm;
    reportado[ClaveUmbralLuz] = configuracion_.umbralLuzPorcentaje;
    reportado[ClaveSolicitudPartida] = solicitudPartida_;
    reportado["longitudSecuencia"] = parametros.longitudSecuencia;
    reportado["toleranciaCm"] = parametros.toleranciaCm;
    reportado["estadoPartida"] = textoEstadoPartida(estadoPartida_);
    reportado["desafioActual"] = desafioActual_;
    reportado["idPartida"] = idPartida_;
    if (esDistanciaReportable(distanciaCm_)) {
        reportado["distanciaCm"] = distanciaCm_;
    }
    if (esLuzReportable(luzPorcentaje_)) {
        reportado["luzPorcentaje"] = luzPorcentaje_;
    }
    reportado["codigoVinculacion"] = codigoVinculacion_;
    reportado["firmware"] = versionFirmware_;

    // measureJson no cuenta el terminador: hace falta un byte mas.
    if (measureJson(documento) >= capacidad) {
        return false;
    }
    serializeJson(documento, destino, capacidad);
    return true;
}

bool EstadoShadow::leerOrdenesDelta(const char* json, OrdenesShadow& ordenes) {
    JsonDocument filtro;
    agregarAtributosAlFiltro(filtro["state"].to<JsonObject>());
    JsonDocument documento;
    if (!deserializarConFiltro(json, filtro, documento)) {
        return false;
    }
    ordenes = interpretarOrdenes(documento["state"].as<JsonObjectConst>());
    return true;
}

bool EstadoShadow::leerOrdenesDocumentoCompleto(const char* json, OrdenesShadow& ordenes) {
    JsonDocument filtro;
    agregarAtributosAlFiltro(filtro["state"]["delta"].to<JsonObject>());
    JsonDocument documento;
    if (!deserializarConFiltro(json, filtro, documento)) {
        return false;
    }
    ordenes = interpretarOrdenes(documento["state"]["delta"].as<JsonObjectConst>());
    return true;
}

const char* EstadoShadow::textoCerradura(EstadoCerradura estado) {
    return estado == EstadoCerradura::Abierta ? TextoAbierta : TextoCerrada;
}

const char* EstadoShadow::textoModo(ModoCaja modo) {
    return modo == ModoCaja::Manual ? TextoManual : TextoJuego;
}

const char* EstadoShadow::textoEstadoPartida(EstadoPartida estado) {
    switch (estado) {
        case EstadoPartida::EnCurso:
            return TextoEnCurso;
        case EstadoPartida::Resuelta:
            return TextoResuelta;
        case EstadoPartida::Esperando:
        default:
            return TextoEsperando;
    }
}
