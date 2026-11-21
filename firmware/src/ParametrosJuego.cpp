#include "ParametrosJuego.h"

#include <string.h>

namespace {

constexpr char TextoFacil[] = "facil";
constexpr char TextoNormal[] = "normal";
constexpr char TextoDificil[] = "dificil";

}  // namespace

ParametrosJuego ParametrosJuego::armar(Dificultad dificultad, uint8_t distanciaObjetivoCm,
                                       uint8_t umbralLuzPorcentaje) {
    return ParametrosJuego{longitudSecuenciaDe(dificultad), toleranciaCmDe(dificultad),
                           distanciaObjetivoCm, umbralLuzPorcentaje};
}

ParametrosJuego ParametrosJuego::porDefecto() {
    return armar(DificultadPorDefecto, DistanciaObjetivoPorDefectoCm,
                 UmbralLuzPorDefectoPorcentaje);
}

uint8_t ParametrosJuego::longitudSecuenciaDe(Dificultad dificultad) {
    switch (dificultad) {
        case Dificultad::Facil:
            return 3;
        case Dificultad::Dificil:
            return 6;
        case Dificultad::Normal:
        default:
            return 4;
    }
}

uint8_t ParametrosJuego::toleranciaCmDe(Dificultad dificultad) {
    switch (dificultad) {
        case Dificultad::Facil:
            return 6;
        case Dificultad::Dificil:
            return 2;
        case Dificultad::Normal:
        default:
            return 4;
    }
}

bool ParametrosJuego::esDistanciaObjetivoValida(int32_t distanciaCm) {
    return distanciaCm >= DistanciaObjetivoMinimaCm && distanciaCm <= DistanciaObjetivoMaximaCm;
}

bool ParametrosJuego::esUmbralLuzValido(int32_t porcentaje) {
    return porcentaje >= UmbralLuzMinimoPorcentaje && porcentaje <= UmbralLuzMaximoPorcentaje;
}

const char* ParametrosJuego::textoDificultad(Dificultad dificultad) {
    switch (dificultad) {
        case Dificultad::Facil:
            return TextoFacil;
        case Dificultad::Dificil:
            return TextoDificil;
        case Dificultad::Normal:
        default:
            return TextoNormal;
    }
}

bool ParametrosJuego::leerDificultad(const char* texto, Dificultad& dificultad) {
    if (texto == nullptr) {
        return false;
    }
    if (strcmp(texto, TextoFacil) == 0) {
        dificultad = Dificultad::Facil;
        return true;
    }
    if (strcmp(texto, TextoNormal) == 0) {
        dificultad = Dificultad::Normal;
        return true;
    }
    if (strcmp(texto, TextoDificil) == 0) {
        dificultad = Dificultad::Dificil;
        return true;
    }
    return false;
}

ParametrosJuego ConfiguracionJuego::parametros() const {
    return ParametrosJuego::armar(dificultad, distanciaObjetivoCm, umbralLuzPorcentaje);
}

bool ConfiguracionJuego::aceptaOrdenesDeCerradura() const {
    return modo == ModoCaja::Manual;
}

ConfiguracionJuego ConfiguracionJuego::desdeValoresGuardados(uint8_t modo, uint8_t dificultad,
                                                             uint8_t distanciaObjetivoCm,
                                                             uint8_t umbralLuzPorcentaje) {
    ConfiguracionJuego configuracion;  // Arranca con los valores de fabrica.
    if (modo <= static_cast<uint8_t>(ModoCaja::Manual)) {
        configuracion.modo = static_cast<ModoCaja>(modo);
    }
    if (dificultad <= static_cast<uint8_t>(Dificultad::Dificil)) {
        configuracion.dificultad = static_cast<Dificultad>(dificultad);
    }
    if (ParametrosJuego::esDistanciaObjetivoValida(distanciaObjetivoCm)) {
        configuracion.distanciaObjetivoCm = distanciaObjetivoCm;
    }
    if (ParametrosJuego::esUmbralLuzValido(umbralLuzPorcentaje)) {
        configuracion.umbralLuzPorcentaje = umbralLuzPorcentaje;
    }
    return configuracion;
}

bool operator==(const ConfiguracionJuego& una, const ConfiguracionJuego& otra) {
    return una.modo == otra.modo && una.dificultad == otra.dificultad &&
           una.distanciaObjetivoCm == otra.distanciaObjetivoCm &&
           una.umbralLuzPorcentaje == otra.umbralLuzPorcentaje;
}

bool operator!=(const ConfiguracionJuego& una, const ConfiguracionJuego& otra) {
    return !(una == otra);
}
