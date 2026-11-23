"""Validación de parámetros de calibración antes de escribir desired."""

import re

from ask_sdk_core.utils import get_slot

from manejadores.juego_comun import (
    IntentCajaHandler, ORDEN_PENDIENTE, resolver_id_slot,
)
from servicios.servicio_shadow import RANGOS_PARAMETROS


PATRON_ENTERO = re.compile(r"-?[0-9]+")
DESCRIPCIONES = {
    "distanciaObjetivoCm": "distancia objetivo en centímetros",
    "umbralLuzPorcentaje": "umbral de luz en porcentaje",
}


class CambiarParametroIntentHandler(IntentCajaHandler):
    """Usa el ID del parámetro y un entero dentro del rango acordado."""

    nombre_intent = "CambiarParametroIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        parametro = resolver_id_slot(
            handler_input, "parametro", RANGOS_PARAMETROS
        )
        if parametro is None:
            return "Indica distancia objetivo o umbral de luz."
        slot = get_slot(handler_input, "valor")
        texto = getattr(slot, "value", None)
        minimo, maximo = RANGOS_PARAMETROS[parametro]
        if (not isinstance(texto, str)
                or not PATRON_ENTERO.fullmatch(texto)
                or not minimo <= int(texto) <= maximo):
            return (f"La {DESCRIPCIONES[parametro]} debe ser un número entero "
                    f"entre {minimo} y {maximo}.")
        self.shadow.pedir_parametro(thing_name, parametro, int(texto))
        return ORDEN_PENDIENTE
