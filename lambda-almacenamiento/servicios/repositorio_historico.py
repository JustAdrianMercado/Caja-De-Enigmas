"""Adaptador mínimo de PutItem, con la tabla boto3 inyectable."""


class RepositorioHistorico:
    """Escribe un ítem sin lecturas previas ni cambios al esquema de claves."""

    def __init__(self, tabla):
        self.tabla = tabla

    def guardar(self, item):
        """Propaga los fallos para no confundirlos con escrituras exitosas."""
        self.tabla.put_item(Item=item)
