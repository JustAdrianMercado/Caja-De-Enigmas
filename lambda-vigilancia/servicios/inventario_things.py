"""Lista de cajas registradas en AWS IoT, para revisarlas todas aunque nunca hayan reportado."""


# Máximo que acepta ListThings por página.
COSAS_POR_PAGINA = 250


class InventarioThings:
    """Nombres de los Things que empiezan con el prefijo de las cajas (p. ej., caja-enigmas-)."""

    def __init__(self, cliente_iot, prefijo_thing):
        self.cliente_iot = cliente_iot
        self.prefijo_thing = prefijo_thing

    def listar_cajas(self):
        """ListThings no filtra por prefijo: se recorren todas las páginas y se filtra aquí."""
        nombres = []
        argumentos = {"maxResults": COSAS_POR_PAGINA}
        while True:
            respuesta = self.cliente_iot.list_things(**argumentos)
            nombres.extend(
                cosa["thingName"] for cosa in respuesta.get("things", [])
                if cosa["thingName"].startswith(self.prefijo_thing)
            )
            token = respuesta.get("nextToken")
            if not token:
                return sorted(nombres)
            argumentos["nextToken"] = token
