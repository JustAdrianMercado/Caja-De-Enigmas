"""Parámetros confirmados, sin calcular valores que aún no se reportaron."""

from manejadores.juego_comun import IntentCajaHandler, NOMBRES_DIFICULTAD


ETIQUETAS = {
    "longitudSecuencia": "longitud de secuencia",
    "toleranciaCm": "tolerancia en centímetros",
    "distanciaObjetivoCm": "distancia objetivo en centímetros",
    "umbralLuzPorcentaje": "umbral de luz en porcentaje",
}


class ConsultarParametrosIntentHandler(IntentCajaHandler):
    """Informa los campos disponibles y advierte si el reporte es parcial."""

    nombre_intent = "ConsultarParametrosIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        parametros = self.shadow.leer_parametros(thing_name)
        if not parametros:
            return "No tengo parámetros reportados por la caja."
        partes = []
        dificultad = parametros.get("dificultad")
        if dificultad in NOMBRES_DIFICULTAD:
            partes.append(f"dificultad {NOMBRES_DIFICULTAD[dificultad]}")
        for campo, etiqueta in ETIQUETAS.items():
            if campo in parametros:
                partes.append(f"{etiqueta}: {parametros[campo]}")
        mensaje = "Según el último reporte, " + "; ".join(partes) + "."
        if len(partes) < len(ETIQUETAS) + 1:
            mensaje += " El reporte de parámetros está incompleto."
        return mensaje
