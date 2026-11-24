"""Pistas con contenido externo pendiente de integración con Lucía."""

from xml.sax.saxutils import escape

from manejadores.juego_comun import IntentCajaHandler, NOMBRES_DESAFIO
from manejadores.identidad import obtener_user_id


ATRIBUTO_PISTAS = "progresoPistas"
PRIMER_NIVEL = 1
ULTIMO_NIVEL = 3


class PedirPistaIntentHandler(IntentCajaHandler):
    """Exige partida en curso, contenido real y publicador disponible."""

    nombre_intent = "PedirPistaIntent"

    def __init__(self, repositorio, servicio_shadow, publicador=None,
                 obtener_pista=None, obtener_pista_progresiva=None):
        super().__init__(repositorio, servicio_shadow)
        self.publicador = publicador
        self.obtener_pista = obtener_pista
        self.obtener_pista_progresiva = obtener_pista_progresiva

    def mensaje_para_caja(self, handler_input, thing_name):
        reportado = self.shadow.leer_reported(thing_name)
        if reportado.get("estadoPartida") in ("esperando", "resuelta"):
            return "No hay una partida en curso para pedir una pista."
        desafio = reportado.get("desafioActual")
        if (reportado.get("estadoPartida") != "en_curso"
                or desafio not in NOMBRES_DESAFIO):
            return "No tengo un reporte válido del desafío en curso."
        if self.publicador is None:
            return "Las pistas todavía no están disponibles."
        if self.obtener_pista_progresiva is not None:
            return self.entregar_pista_progresiva(
                handler_input, thing_name, reportado, desafio
            )
        if self.obtener_pista is None:
            return "Las pistas todavía no están disponibles."
        pista = self.obtener_pista(desafio)
        if not isinstance(pista, str) or not pista.strip():
            return "No hay una pista disponible para ese desafío."
        self.publicador.publicar_pista(thing_name, desafio)
        return escape(pista)

    def entregar_pista_progresiva(self, handler_input, thing_name,
                                 reportado, desafio):
        sesion = handler_input.request_envelope.session
        id_partida = reportado.get("idPartida")
        if (sesion is None or not sesion.session_id
                or type(id_partida) is not int or id_partida < 0):
            return "No tengo una sesión y una partida reportada válidas."
        contexto = {
            "usuario": obtener_user_id(handler_input),
            "caja": thing_name, "sesion": sesion.session_id,
            "partida": id_partida, "desafio": desafio,
        }
        gestor = handler_input.attributes_manager
        atributos = gestor.session_attributes
        nivel = self.siguiente_nivel(atributos, contexto, sesion.new)
        pista = self.obtener_pista_progresiva(desafio, nivel)
        if not isinstance(pista, str) or not pista.strip():
            return "No hay una pista disponible para ese desafío."

        # Prepara el estado antes del efecto externo. Si el SDK falla al
        # guardarlo, no publica; si publicar falla, conserva el nivel anterior.
        nuevos_atributos = dict(atributos)
        nuevos_atributos[ATRIBUTO_PISTAS] = {
            "contexto": contexto, "nivel": nivel,
        }
        gestor.session_attributes = nuevos_atributos
        try:
            self.publicador.publicar_pista(thing_name, desafio)
        except Exception:
            gestor.session_attributes = atributos
            raise
        return escape(pista)

    @staticmethod
    def siguiente_nivel(atributos, contexto, es_sesion_nueva):
        if not isinstance(atributos, dict):
            raise ValueError("Atributos de sesión inválidos.")
        anterior = atributos.get(ATRIBUTO_PISTAS)
        if es_sesion_nueva or anterior is None:
            return PRIMER_NIVEL
        if not isinstance(anterior, dict):
            raise ValueError("Progreso de pistas inválido.")
        if anterior.get("contexto") != contexto:
            return PRIMER_NIVEL
        nivel = anterior.get("nivel")
        if type(nivel) is not int or not PRIMER_NIVEL <= nivel <= ULTIMO_NIVEL:
            raise ValueError("Nivel de pista inválido.")
        return min(nivel + 1, ULTIMO_NIVEL)
