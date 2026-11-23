"""Identidad, respuestas y slots compartidos por los intents del juego."""

from ask_sdk_core.dispatch_components import AbstractRequestHandler
from ask_sdk_core.utils import get_slot, is_intent_name

from manejadores.identidad import obtener_user_id
from textos import REPREGUNTA, SIN_USUARIO, SIN_VINCULACION


ORDEN_PENDIENTE = (
    "Envié la solicitud. Está pendiente de confirmación de la caja; "
    "consulta el estado reportado para comprobar el resultado."
)
NOMBRES_DIFICULTAD = {
    "facil": "fácil", "normal": "normal", "dificil": "difícil",
}
NOMBRES_DESAFIO = {
    "memoria": "memoria", "distancia": "distancia", "luz": "luz",
}


def resolver_id_slot(handler_input, nombre, permitidos):
    """Exige una resolución inequívoca; nunca usa el texto pronunciado."""
    slot = get_slot(handler_input, nombre)
    resoluciones = getattr(slot, "resolutions", None)
    autoridades = getattr(resoluciones, "resolutions_per_authority", None)
    identificadores = set()
    for autoridad in autoridades or []:
        codigo = getattr(getattr(autoridad, "status", None), "code", None)
        if getattr(codigo, "value", codigo) != "ER_SUCCESS_MATCH":
            continue
        for resultado in autoridad.values or []:
            identificador = getattr(resultado.value, "id", None)
            if identificador not in permitidos:
                return None
            identificadores.add(identificador)
    if len(identificadores) != 1:
        return None
    return identificadores.pop()


class IntentCajaHandler(AbstractRequestHandler):
    """Resuelve siempre la caja del usuario antes de acceder a servicios."""

    nombre_intent = ""

    def __init__(self, repositorio, servicio_shadow):
        self.repositorio = repositorio
        self.shadow = servicio_shadow

    def can_handle(self, handler_input):
        return is_intent_name(self.nombre_intent)(handler_input)

    def handle(self, handler_input):
        usuario = obtener_user_id(handler_input)
        if not usuario:
            return self.responder(handler_input, SIN_USUARIO)
        vinculacion = self.repositorio.obtener(usuario)
        if not vinculacion:
            return self.responder(handler_input, SIN_VINCULACION)
        mensaje = self.mensaje_para_caja(
            handler_input, vinculacion["thingName"]
        )
        return self.responder(handler_input, mensaje)

    @staticmethod
    def responder(handler_input, mensaje):
        return (handler_input.response_builder.speak(mensaje)
                .ask(REPREGUNTA).response)

    def mensaje_para_caja(self, handler_input, thing_name):
        raise NotImplementedError
