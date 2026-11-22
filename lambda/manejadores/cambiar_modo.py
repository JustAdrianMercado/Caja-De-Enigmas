"""Cambio del modo mediante el ID resuelto por Alexa."""

from manejadores.juego_comun import (
    IntentCajaHandler, ORDEN_PENDIENTE, resolver_id_slot,
)
from servicios.servicio_shadow import MODOS


class CambiarModoIntentHandler(IntentCajaHandler):
    """Delega al servicio el parche de modo y la limpieza de cerradura."""

    nombre_intent = "CambiarModoIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        modo = resolver_id_slot(handler_input, "modo", MODOS)
        if modo is None:
            return "Indica el modo juego o manual."
        self.shadow.pedir_modo(thing_name, modo)
        return ORDEN_PENDIENTE
