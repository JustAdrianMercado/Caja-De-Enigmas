"""Lectura y escritura de CajaEnigmasHistorico para la vigilancia (patrón de acceso 5)."""

from vigilancia import TIPO_ALERTA, TIPO_LECTURA, UltimoRegistro


# Ítems que se leen por página. Una caja activa tiene su última lectura entre los últimos
# eventos, así que casi siempre alcanza con la primera página.
TAMANO_PAGINA = 25
# Tope de páginas por caja, para acotar el costo si una caja acumula muchos eventos sin
# lecturas. 40 páginas = 1000 ítems revisados como máximo.
MAXIMO_PAGINAS = 40


class RepositorioHistorico:
    """Busca la última lectura o alerta de una caja y guarda las alertas."""

    def __init__(self, tabla):
        self.tabla = tabla

    def ultimo_registro(self, thing_name):
        """La lectura o alerta más nueva de la caja, o None si no tiene ninguna."""
        argumentos = {
            "KeyConditionExpression": "thingName = :thing",
            "FilterExpression": "#tipo IN (:lectura, :alerta)",
            "ExpressionAttributeNames": {"#tipo": "tipo"},
            "ExpressionAttributeValues": {
                ":thing": thing_name, ":lectura": TIPO_LECTURA, ":alerta": TIPO_ALERTA,
            },
            "ScanIndexForward": False,  # De la más nueva a la más vieja.
            "Limit": TAMANO_PAGINA,
        }
        for _ in range(MAXIMO_PAGINAS):
            respuesta = self.tabla.query(**argumentos)
            items = respuesta.get("Items", [])
            if items:
                return convertir_item(items[0])
            clave_siguiente = respuesta.get("LastEvaluatedKey")
            if clave_siguiente is None:
                return None
            argumentos["ExclusiveStartKey"] = clave_siguiente
        return None

    def guardar_alerta(self, alerta):
        """Propaga los fallos: una alerta perdida no se puede confundir con una guardada."""
        self.tabla.put_item(Item=alerta)


def convertir_item(item):
    """DynamoDB devuelve los números como Decimal; la fecha es el prefijo de "ts"."""
    tipo = item["tipo"]
    campo_momento = "tsRegla" if tipo == TIPO_LECTURA else "tsGuardado"
    return UltimoRegistro(
        tipo=tipo,
        fecha=item["ts"].split("#", 1)[0],
        momento_ms=int(item[campo_momento]),
    )
