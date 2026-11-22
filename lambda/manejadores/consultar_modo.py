"""Consulta exclusivamente el modo reportado por la caja."""

from manejadores.juego_comun import IntentCajaHandler


class ConsultarModoIntentHandler(IntentCajaHandler):
    """No confirma una orden pendiente usando desired."""

    nombre_intent = "ConsultarModoIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        modo = self.shadow.leer_modo(thing_name)
        if modo not in ("juego", "manual"):
            return "No tengo información reportada sobre el modo."
        return f"Según el último reporte, la caja está en modo {modo}."
