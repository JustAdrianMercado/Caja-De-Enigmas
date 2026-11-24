"""Consulta las mediciones del último reporte de la caja vinculada."""

from manejadores.juego_comun import IntentCajaHandler


# Rangos de medición del contrato v2, sección 2; no son metas del juego.
DISTANCIA_MINIMA_CM = 2
DISTANCIA_MAXIMA_CM = 400
LUZ_MINIMA_PORCENTAJE = 0
LUZ_MAXIMA_PORCENTAJE = 100


def es_lectura_valida(valor, minimo, maximo):
    """Exige enteros del contrato; bool es una subclase de int en Python."""
    return type(valor) is int and minimo <= valor <= maximo


class ConsultarSensoresIntentHandler(IntentCajaHandler):
    """Informa cada medición válida sin sustituir las lecturas ausentes."""

    nombre_intent = "ConsultarSensoresIntent"

    def mensaje_para_caja(self, handler_input, thing_name):
        reportado = self.shadow.leer_reported(thing_name)
        distancia_cm = reportado.get("distanciaCm")
        luz_porcentaje = reportado.get("luzPorcentaje")
        hay_distancia = es_lectura_valida(
            distancia_cm, DISTANCIA_MINIMA_CM, DISTANCIA_MAXIMA_CM
        )
        hay_luz = es_lectura_valida(
            luz_porcentaje, LUZ_MINIMA_PORCENTAJE, LUZ_MAXIMA_PORCENTAJE
        )
        if not hay_distancia and not hay_luz:
            return "No tengo lecturas válidas de distancia ni de luz de tu caja."

        partes = ["Según el último reporte de tu caja:"]
        if hay_distancia:
            partes.append(f"la distancia es de {distancia_cm} centímetros.")
        else:
            partes.append("No tengo una lectura válida de distancia.")
        if hay_luz:
            partes.append(f"La luz es de {luz_porcentaje} por ciento.")
        else:
            partes.append("No tengo una lectura válida de luz.")
        return " ".join(partes)
