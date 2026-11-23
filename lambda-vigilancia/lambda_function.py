"""Entrada de caja-enigmas-vigilancia: la invoca EventBridge cada 5 minutos.

Solo arma las dependencias: la lógica está en vigilancia.py.
"""

from functools import lru_cache
import json
import logging
import os
import time

from servicios.inventario_things import InventarioThings
from servicios.repositorio_historico import RepositorioHistorico
from vigilancia import VigilanciaCajas


REGISTRADOR = logging.getLogger(__name__)
REGISTRADOR.setLevel(logging.INFO)
REGION = "us-east-1"
TABLA_POR_DEFECTO = "CajaEnigmasHistorico"
PREFIJO_POR_DEFECTO = "caja-enigmas-"
MINUTOS_SIN_REPORTE_POR_DEFECTO = 15
NANOSEGUNDOS_POR_MILISEGUNDO = 1_000_000


def hora_actual_ms():
    """Reloj UTC de producción; los tests inyectan uno fijo."""
    return time.time_ns() // NANOSEGUNDOS_POR_MILISEGUNDO


def leer_configuracion(entorno):
    """Variables de entorno de la sección 6 del contrato. Para la demo se baja el límite."""
    texto_minutos = entorno.get(
        "MINUTOS_SIN_REPORTE", str(MINUTOS_SIN_REPORTE_POR_DEFECTO)
    )
    try:
        minutos = int(texto_minutos)
    except ValueError:
        raise RuntimeError("MINUTOS_SIN_REPORTE debe ser un entero.") from None
    if minutos <= 0:
        raise RuntimeError("MINUTOS_SIN_REPORTE debe ser mayor que cero.")
    return {
        "tabla": entorno.get("TABLA_HISTORICO", TABLA_POR_DEFECTO),
        "prefijo": entorno.get("PREFIJO_THING", PREFIJO_POR_DEFECTO),
        "minutos": minutos,
    }


@lru_cache(maxsize=1)
def crear_vigilancia():
    """Solo importa boto3 en AWS: los tests no lo necesitan."""
    import boto3

    configuracion = leer_configuracion(os.environ)
    tabla = boto3.resource("dynamodb", region_name=REGION).Table(configuracion["tabla"])
    cliente_iot = boto3.client("iot", region_name=REGION)
    return VigilanciaCajas(
        inventario=InventarioThings(cliente_iot, configuracion["prefijo"]),
        historico=RepositorioHistorico(tabla),
        minutos_sin_reporte=configuracion["minutos"],
        reloj_ms=hora_actual_ms,
    )


def lambda_handler(event, context):
    """El evento de EventBridge no trae datos útiles: cada invocación revisa todas las cajas."""
    resumen = crear_vigilancia().revisar()
    REGISTRADOR.info("%s", json.dumps(resumen, ensure_ascii=True))
    return resumen
