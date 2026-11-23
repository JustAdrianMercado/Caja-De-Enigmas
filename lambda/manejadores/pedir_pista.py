"""Pistas con contenido externo pendiente de integración con Lucía."""

from xml.sax.saxutils import escape

from manejadores.juego_comun import IntentCajaHandler, NOMBRES_DESAFIO


class PedirPistaIntentHandler(IntentCajaHandler):
    """Exige partida en curso, contenido real y publicador disponible."""

    nombre_intent = "PedirPistaIntent"

    def __init__(self, repositorio, servicio_shadow, publicador=None,
                 obtener_pista=None):
        super().__init__(repositorio, servicio_shadow)
        self.publicador = publicador
        self.obtener_pista = obtener_pista

    def mensaje_para_caja(self, handler_input, thing_name):
        reportado = self.shadow.leer_reported(thing_name)
        if reportado.get("estadoPartida") in ("esperando", "resuelta"):
            return "No hay una partida en curso para pedir una pista."
        desafio = reportado.get("desafioActual")
        if (reportado.get("estadoPartida") != "en_curso"
                or desafio not in NOMBRES_DESAFIO):
            return "No tengo un reporte válido del desafío en curso."
        if self.obtener_pista is None or self.publicador is None:
            return "Las pistas todavía no están disponibles."
        pista = self.obtener_pista(desafio)
        if not isinstance(pista, str) or not pista.strip():
            return "No hay una pista disponible para ese desafío."
        self.publicador.publicar_pista(thing_name, desafio)
        return escape(pista)
