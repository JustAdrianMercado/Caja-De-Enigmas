#include "ColaEventos.h"

#include <string.h>

bool ColaEventos::agregar(const char* json) {
    if (json == nullptr || json[0] == '\0') {
        return false;
    }
    const size_t longitud = strlen(json);
    if (longitud >= EventoJson::TamanoMaximo) {
        return false;
    }
    if (cantidad_ == Capacidad) {
        descartarPrimero();  // Cola llena: se pierde el mas viejo, no el mas nuevo.
    }
    const size_t indiceNuevo = (indicePrimero_ + cantidad_) % Capacidad;
    memcpy(eventos_[indiceNuevo], json, longitud + 1);  // Con el terminador.
    ++cantidad_;
    return true;
}

bool ColaEventos::estaVacia() const {
    return cantidad_ == 0;
}

size_t ColaEventos::cantidad() const {
    return cantidad_;
}

const char* ColaEventos::primero() const {
    if (estaVacia()) {
        return nullptr;
    }
    return eventos_[indicePrimero_];
}

void ColaEventos::quitarPrimero() {
    if (estaVacia()) {
        return;
    }
    indicePrimero_ = (indicePrimero_ + 1) % Capacidad;
    --cantidad_;
    fallosDelPrimero_ = 0;
}

bool ColaEventos::registrarFalloDelPrimero() {
    if (estaVacia()) {
        return false;
    }
    ++fallosDelPrimero_;
    if (fallosDelPrimero_ < FallosAntesDeDescartar) {
        return false;
    }
    descartarPrimero();
    return true;
}

uint32_t ColaEventos::descartados() const {
    return descartados_;
}

void ColaEventos::descartarPrimero() {
    quitarPrimero();
    ++descartados_;
}
