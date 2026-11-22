"""Construcción pura de ítems del histórico acordado en el contrato v2."""

from validacion import CAMPOS_EVENTO, CAMPOS_LECTURA, fecha_desde_epoch_ms


DIAS_RETENCION_LECTURAS = 30
SEGUNDOS_POR_DIA = 86400
MILISEGUNDOS_POR_SEGUNDO = 1000


def construir_item(mensaje, ts_guardado_ms,
                   dias_retencion=DIAS_RETENCION_LECTURAS):
    """Recibe el mensaje validado y la hora previa al PutItem."""
    if type(dias_retencion) is not int or dias_retencion <= 0:
        raise ValueError("La retención debe ser un entero positivo de días.")
    es_evento = mensaje.get("tipoMensaje") == "evento"
    sufijo = mensaje["tipo"] if es_evento else "lectura"
    fecha = mensaje.get("tsObjeto")
    if fecha is None:
        fecha = fecha_desde_epoch_ms(mensaje["tsRegla"]).isoformat(
            timespec="milliseconds"
        ).replace("+00:00", "Z")
    item = {
        "thingName": mensaje["thingName"], "ts": f"{fecha}#{sufijo}",
        "tipo": "evento" if es_evento else "lectura",
        "tsRegla": mensaje["tsRegla"], "tsGuardado": ts_guardado_ms,
    }
    campos = CAMPOS_EVENTO if es_evento else CAMPOS_LECTURA
    item.update({
        campo: mensaje[campo] for campo in campos if campo in mensaje
    })
    if "tsObjeto" in mensaje:
        item["tsObjeto"] = mensaje["tsObjeto"]
    if es_evento:
        item["evento"] = mensaje["tipo"]
    else:
        # Retención desde la recepción: una lectura retrasada conserva 30 días.
        item["expiraEn"] = (
            ts_guardado_ms // MILISEGUNDOS_POR_SEGUNDO
            + dias_retencion * SEGUNDOS_POR_DIA
        )
    return item
