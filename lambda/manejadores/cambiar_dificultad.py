"""Cambio de dificultad usando los IDs del contrato."""

from manejadores.juego_comun import (
    IntentCajaHandler, ORDEN_PENDIENTE, resolver_id_slot,
)
from servicios.servicio_shadow import DIFICULTADES


class CambiarDificultadIntentHandler(IntentCajaHandler):
    """Escribe dificultad sin modificar los parámetros de calibración."""

    nombre_intent = "CambiarDificultadIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        dificultad = resolver_id_slot(
            handler_input, "dificultad", DIFICULTADES
        )
        if dificultad is None:
            return "Indica dificultad fácil, normal o difícil."
        self.shadow.pedir_dificultad(thing_name, dificultad)
        return ORDEN_PENDIENTE
