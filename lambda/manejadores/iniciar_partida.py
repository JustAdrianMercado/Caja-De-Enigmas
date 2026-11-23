"""Solicitud de partida basada en el contador reportado del contrato v2."""

from manejadores.juego_comun import IntentCajaHandler, ORDEN_PENDIENTE


class IniciarPartidaIntentHandler(IntentCajaHandler):
    """Rechaza manual y los reportes incompletos antes de pedir la partida."""

    nombre_intent = "IniciarPartidaIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        reportado = self.shadow.leer_reported(thing_name)
        if reportado.get("modo") == "manual":
            return ("La caja está en modo manual. "
                    "Pásala a modo juego para iniciar.")
        contador = reportado.get("solicitudPartida")
        if (reportado.get("modo") != "juego"
                or type(contador) is not int or contador < 0):
            return "Falta un modo juego y un contador válidos en el reporte."
        self.shadow.pedir_partida(thing_name)
        return ORDEN_PENDIENTE
