"""Validación y selección de campos del contrato v2, sin servicios externos."""

from datetime import datetime, timedelta, timezone
import re


PATRON_THING = re.compile(r"caja-enigmas-[0-9]{2}")
PATRON_FECHA = re.compile(
    r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}Z"
)
EPOCH = datetime(1970, 1, 1, tzinfo=timezone.utc)
# Último milisegundo representable por datetime (31/12/9999 UTC).
EPOCH_MAXIMO_MS = 253402300799999
RANGOS = {
    "distanciaCm": (2, 400), "luzPorcentaje": (0, 100),
    "idPartida": (1, None), "duracionMs": (0, 3600000),
    "intentos": (1, 99),
}
VALORES = {
    "desafio": ("memoria", "distancia", "luz"),
    "origen": ("voz", "edge", "nube"),
    "dificultad": ("facil", "normal", "dificil"),
    "cerradura": ("abierta", "cerrada"),
}
CAMPOS_POR_EVENTO = {
    "partida_iniciada": ("idPartida", "origen", "dificultad"),
    "desafio_superado": ("idPartida", "desafio", "duracionMs", "intentos"),
    "desafio_fallado": ("idPartida", "desafio"),
    "partida_resuelta": ("idPartida", "duracionMs"),
    "cerradura": ("cerradura", "origen"),
    "pista": ("desafio", "origen"),
}
CAMPOS_LECTURA = ("distanciaCm", "luzPorcentaje")
CAMPOS_EVENTO = ("idPartida", "duracionMs", "intentos", *VALORES)


class MensajeInvalido(ValueError):
    """Motivo controlado de descarte, sin incluir valores del publicador."""


def validar_entero(valor, campo, minimo, maximo=None):
    """Excluye bool, que Python considera una subclase de int."""
    if type(valor) is not int or valor < minimo:
        raise MensajeInvalido(f"{campo}: entero fuera de rango")
    if maximo is not None and valor > maximo:
        raise MensajeInvalido(f"{campo}: entero fuera de rango")


def fecha_desde_epoch_ms(epoch_ms):
    """Convierte sin floats para conservar exactamente los milisegundos."""
    validar_entero(epoch_ms, "tsRegla", 0, EPOCH_MAXIMO_MS)
    return EPOCH + timedelta(milliseconds=epoch_ms)


def leer_fecha_objeto(valor):
    """Exige fecha real y formato UTC con exactamente tres decimales."""
    if not isinstance(valor, str) or not PATRON_FECHA.fullmatch(valor):
        raise MensajeInvalido("tsObjeto: formato UTC con ms inválido")
    try:
        return datetime.fromisoformat(valor.replace("Z", "+00:00"))
    except ValueError:
        raise MensajeInvalido("tsObjeto: fecha inválida") from None


def seleccionar_campos(mensaje, campos):
    """Valida campos conocidos; nunca conserva atributos arbitrarios."""
    seleccion = {}
    for campo in campos:
        if campo not in mensaje:
            continue
        valor = mensaje[campo]
        if campo in RANGOS:
            validar_entero(valor, campo, *RANGOS[campo])
        elif not isinstance(valor, str) or valor not in VALORES[campo]:
            raise MensajeInvalido(f"{campo}: valor inválido")
        seleccion[campo] = valor
    return seleccion


def validar_contenido(mensaje):
    """Reconoce las dos salidas SQL de §6, sin confundir tipo y tipoMensaje."""
    tipo = mensaje.get("tipo")
    if tipo == "lectura" and "tipoMensaje" not in mensaje:
        campos = seleccionar_campos(mensaje, CAMPOS_LECTURA)
        if not campos:
            raise MensajeInvalido("lectura: faltan sensores")
        return campos
    if (mensaje.get("tipoMensaje") != "evento"
            or not isinstance(tipo, str) or tipo not in CAMPOS_POR_EVENTO):
        raise MensajeInvalido("tipo de mensaje desconocido")
    for campo in CAMPOS_POR_EVENTO[tipo]:
        if campo not in mensaje:
            raise MensajeInvalido(f"{campo}: campo obligatorio ausente")
    campos = seleccionar_campos(mensaje, CAMPOS_EVENTO)
    if tipo == "pista" and campos["origen"] != "voz":
        raise MensajeInvalido("pista: origen debe ser voz")
    if tipo == "partida_iniciada" and campos["origen"] not in ("edge", "voz"):
        raise MensajeInvalido("partida_iniciada: origen debe ser edge o voz")
    return campos


def validar_mensaje(mensaje):
    """Devuelve una copia validada con los metadatos de la regla."""
    if not isinstance(mensaje, dict):
        raise MensajeInvalido("se esperaba un objeto JSON")
    nombre = mensaje.get("thingName")
    if not isinstance(nombre, str) or not PATRON_THING.fullmatch(nombre):
        raise MensajeInvalido("thingName: formato inválido")
    fecha_desde_epoch_ms(mensaje.get("tsRegla"))
    campos = validar_contenido(mensaje)
    campos.update(thingName=nombre, tsRegla=mensaje["tsRegla"],
                  tipo=mensaje["tipo"])
    if "tipoMensaje" in mensaje:
        campos["tipoMensaje"] = "evento"
    if "tsObjeto" in mensaje:
        leer_fecha_objeto(mensaje["tsObjeto"])
        campos["tsObjeto"] = mensaje["tsObjeto"]
    return campos
