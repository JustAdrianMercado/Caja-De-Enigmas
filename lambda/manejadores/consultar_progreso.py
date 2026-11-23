"""Consulta del estado y desafío reportados de la partida."""

from manejadores.juego_comun import IntentCajaHandler, NOMBRES_DESAFIO


class ConsultarProgresoIntentHandler(IntentCajaHandler):
    """No deduce el inicio de una partida de una orden en desired."""

    nombre_intent = "ConsultarProgresoIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        reportado = self.shadow.leer_reported(thing_name)
        estado = reportado.get("estadoPartida")
        desafio = reportado.get("desafioActual")
        if estado == "esperando":
            return ("Según el último reporte, "
                    "la caja está esperando una partida.")
        if estado == "resuelta":
            return "Según el último reporte, la partida está resuelta."
        if estado == "en_curso" and desafio in NOMBRES_DESAFIO:
            return ("Según el último reporte, la partida está en curso, "
                    f"en el desafío de {NOMBRES_DESAFIO[desafio]}.")
        return "No tengo un reporte completo y válido del progreso."
