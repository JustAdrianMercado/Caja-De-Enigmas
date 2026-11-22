"""Composición y procesamiento del almacenamiento desde las dos reglas IoT."""

from functools import lru_cache
import json
import logging
import os
import time

from enriquecimiento import construir_item, DIAS_RETENCION_LECTURAS
from servicios.repositorio_historico import RepositorioHistorico
from validacion import MensajeInvalido, validar_mensaje


REGISTRADOR = logging.getLogger(__name__)
REGISTRADOR.setLevel(logging.INFO)
REGION = "us-east-1"
NANOSEGUNDOS_POR_MILISEGUNDO = 1000000


def hora_actual_ms():
    """Reloj UTC de producción; los tests inyectan uno fijo."""
    return time.time_ns() // NANOSEGUNDOS_POR_MILISEGUNDO


@lru_cache(maxsize=1)
def obtener_repositorio():
    """Solo inicializa AWS al procesar un mensaje válido en producción."""
    import boto3

    tabla = boto3.resource("dynamodb", region_name=REGION).Table(
        os.environ.get("TABLA_HISTORICO", "CajaEnigmasHistorico")
    )
    return RepositorioHistorico(tabla)


def registrar_resultado(mensaje, persistido):
    """Solo registra metadatos que ya pasaron la validación."""
    resumen = {
        "recepcion": "registrada", "persistido": persistido,
        "thingName": mensaje["thingName"], "tipo": mensaje["tipo"],
    }
    if "tipoMensaje" in mensaje:
        resumen["tipoMensaje"] = mensaje["tipoMensaje"]
    REGISTRADOR.info("%s", json.dumps(resumen, ensure_ascii=True))


def procesar_mensaje(event, repositorio=None, reloj=None,
                     dias_retencion=DIAS_RETENCION_LECTURAS):
    """Permite probar la cadena completa sin SDK, credenciales ni red."""
    try:
        mensaje = validar_mensaje(event)
    except MensajeInvalido as error:
        REGISTRADOR.warning("Mensaje descartado: %s", error)
        return {"recibido": isinstance(event, dict), "persistido": False,
                "descartado": True}

    registrar_resultado(mensaje, persistido=False)
    try:
        destino = repositorio
        if destino is None:
            destino = obtener_repositorio()
        hora_guardado_ms = (reloj or hora_actual_ms)()
        item = construir_item(mensaje, hora_guardado_ms, dias_retencion)
        destino.guardar(item)
    except Exception:
        # El runtime también registra excepciones: no propagar mensajes del SDK
        # que podrían incluir payloads o endpoints privados.
        REGISTRADOR.error("No se pudo completar la escritura del histórico.")
        raise RuntimeError("Fallo de almacenamiento del histórico.") from None
    registrar_resultado(mensaje, persistido=True)
    return {"recibido": True, "persistido": True, "descartado": False}


def lambda_handler(event, context):
    """Entrada de Lambda; un fallo de escritura provoca fallo de invocación."""
    try:
        dias_retencion = int(os.environ.get(
            "DIAS_RETENCION_LECTURAS", str(DIAS_RETENCION_LECTURAS)
        ))
        if dias_retencion <= 0:
            raise ValueError
    except ValueError:
        raise RuntimeError("Configuración de retención inválida.") from None
    return procesar_mensaje(event, dias_retencion=dias_retencion)
