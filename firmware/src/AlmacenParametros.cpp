#include "AlmacenParametros.h"

#include <Arduino.h>
#include <Preferences.h>

namespace {

// Espacio propio, separado del "caja" de CodigoVinculacion. Maximo 15 caracteres por nombre.
constexpr char EspacioNvs[] = "juego";
constexpr char ClaveModo[] = "modo";
constexpr char ClaveDificultad[] = "dificultad";
constexpr char ClaveDistanciaObjetivo[] = "distanciaCm";
constexpr char ClaveUmbralLuz[] = "umbralLuz";
constexpr char ClaveIdPartida[] = "idPartida";
constexpr char ClaveSolicitudPartida[] = "solicitud";
constexpr bool SoloLectura = true;

// Valor que no es valido en ningun campo: si falta la clave, se usa el de fabrica.
constexpr uint8_t ValorAusente = 0xFF;

void avisarErrorDeEscritura() {
    Serial.println(F("Parametros: no se pudo guardar en la NVS; se pierde al reiniciar."));
}

}  // namespace

ConfiguracionJuego AlmacenParametros::cargarConfiguracion() const {
    Preferences memoria;
    if (!memoria.begin(EspacioNvs, SoloLectura)) {
        return ConfiguracionJuego{};  // Primer arranque: el espacio todavia no existe.
    }
    const uint8_t modo = memoria.getUChar(ClaveModo, ValorAusente);
    const uint8_t dificultad = memoria.getUChar(ClaveDificultad, ValorAusente);
    const uint8_t distanciaObjetivoCm = memoria.getUChar(ClaveDistanciaObjetivo, ValorAusente);
    const uint8_t umbralLuzPorcentaje = memoria.getUChar(ClaveUmbralLuz, ValorAusente);
    memoria.end();
    return ConfiguracionJuego::desdeValoresGuardados(modo, dificultad, distanciaObjetivoCm,
                                                     umbralLuzPorcentaje);
}

void AlmacenParametros::guardarConfiguracion(const ConfiguracionJuego& configuracion) const {
    Preferences memoria;
    if (!memoria.begin(EspacioNvs, !SoloLectura)) {
        avisarErrorDeEscritura();
        return;
    }
    // putUChar devuelve los bytes escritos: 0 indica un error.
    const bool seGuardo =
        memoria.putUChar(ClaveModo, static_cast<uint8_t>(configuracion.modo)) > 0 &&
        memoria.putUChar(ClaveDificultad, static_cast<uint8_t>(configuracion.dificultad)) > 0 &&
        memoria.putUChar(ClaveDistanciaObjetivo, configuracion.distanciaObjetivoCm) > 0 &&
        memoria.putUChar(ClaveUmbralLuz, configuracion.umbralLuzPorcentaje) > 0;
    memoria.end();
    if (!seGuardo) {
        avisarErrorDeEscritura();
    }
}

uint32_t AlmacenParametros::cargarIdPartida() const {
    return cargarContador(ClaveIdPartida);
}

void AlmacenParametros::guardarIdPartida(uint32_t idPartida) const {
    guardarContador(ClaveIdPartida, idPartida);
}

uint32_t AlmacenParametros::cargarSolicitudPartida() const {
    return cargarContador(ClaveSolicitudPartida);
}

void AlmacenParametros::guardarSolicitudPartida(uint32_t solicitudPartida) const {
    guardarContador(ClaveSolicitudPartida, solicitudPartida);
}

uint32_t AlmacenParametros::cargarContador(const char* clave) const {
    Preferences memoria;
    if (!memoria.begin(EspacioNvs, SoloLectura)) {
        return 0;
    }
    const uint32_t valor = memoria.getUInt(clave, 0);
    memoria.end();
    return valor;
}

void AlmacenParametros::guardarContador(const char* clave, uint32_t valor) const {
    Preferences memoria;
    const bool seGuardo =
        memoria.begin(EspacioNvs, !SoloLectura) && memoria.putUInt(clave, valor) > 0;
    memoria.end();  // No hace nada si begin() fallo.
    if (!seGuardo) {
        avisarErrorDeEscritura();
    }
}
