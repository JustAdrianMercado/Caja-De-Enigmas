"""Publicación de pistas en el tema de eventos acordado en el contrato v2."""

import json
import re


PATRON_THING = re.compile(r"caja-enigmas-[0-9]{2}")
DESAFIOS = ("memoria", "distancia", "luz")
QOS_EVENTOS = 1


class PublicadorEventos:
    """Usa un cliente IoT Data inyectado, sin escribir en el Shadow."""

    def __init__(self, cliente_iot):
        self.cliente_iot = cliente_iot

    def publicar_pista(self, thing_name, desafio):
        if (not isinstance(thing_name, str)
                or not PATRON_THING.fullmatch(thing_name)):
            raise ValueError("Nombre de caja inválido para publicar.")
        if desafio not in DESAFIOS:
            raise ValueError("Desafío inválido para publicar pista.")
        mensaje = {"tipo": "pista", "desafio": desafio, "origen": "voz"}
        self.cliente_iot.publish(
            topic=f"caja-enigmas/{thing_name}/eventos", qos=QOS_EVENTOS,
            retain=False, payload=json.dumps(mensaje).encode("utf-8"),
        )
