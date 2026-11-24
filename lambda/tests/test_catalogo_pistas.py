"""Catálogo real mediante el arranque de producción, sin acceso a AWS."""

import copy
import json
import os
import runpy
import unittest
from io import BytesIO
from pathlib import Path
from unittest.mock import Mock, patch

from apoyo import evento_alexa, voz
from pistas import PISTAS_POR_DESAFIO


class TestCatalogoPistas(unittest.TestCase):
    """Ejercita el punto de entrada con servicios reales y clientes simulados."""

    def setUp(self):
        self.reportado = {
            "estadoPartida": "en_curso", "idPartida": 12,
            "desafioActual": "memoria", "modo": "juego",
            "distanciaCm": 63, "luzPorcentaje": 22,
        }
        self.tabla = Mock()
        self.tabla.get_item.return_value = {
            "Item": {"thingName": "caja-enigmas-01"},
        }
        recurso = Mock()
        recurso.Table.return_value = self.tabla
        self.cliente = Mock()
        self.cliente.get_thing_shadow.side_effect = self.leer_shadow
        with (
            patch.dict(os.environ, {
                "IOT_ENDPOINT": "https://iot.example.invalid",
            }),
            patch("boto3.resource", return_value=recurso),
            patch("boto3.client", return_value=self.cliente),
        ):
            modulo = runpy.run_path(str(
                Path(__file__).resolve().parents[1] / "lambda_function.py"
            ))
        self.invocar = modulo["lambda_handler"]
        self.atributos = {}
        self.sesion = "sesion-01"
        self.es_nueva = True

    def leer_shadow(self, **argumentos):
        return {"payload": BytesIO(json.dumps({
            "state": {"reported": self.reportado},
        }).encode("utf-8"))}

    def solicitar(self, intent="PedirPistaIntent"):
        evento = evento_alexa(intent)
        evento["session"].update({
            "sessionId": self.sesion, "new": self.es_nueva,
            "attributes": copy.deepcopy(self.atributos),
        })
        respuesta = self.invocar(evento, None)
        self.atributos = json.loads(json.dumps(respuesta["sessionAttributes"]))
        self.es_nueva = False
        return voz(respuesta)

    def test_catalogo_contiene_exactamente_tres_pistas_por_desafio(self):
        self.assertEqual(set(PISTAS_POR_DESAFIO),
                         {"memoria", "distancia", "luz"})
        for pistas in PISTAS_POR_DESAFIO.values():
            self.assertEqual(len(pistas), 3)
            self.assertEqual(len(set(pistas)), 3)
            for pista in pistas:
                self.assertIsInstance(pista, str)
                self.assertTrue(pista.strip())

    def test_nueve_textos_en_orden_y_tercera_repetida_publican_eventos(self):
        for desafio, pistas in PISTAS_POR_DESAFIO.items():
            self.reportado["desafioActual"] = desafio
            for pista in (*pistas, pistas[-1]):
                with self.subTest(desafio=desafio, pista=pista):
                    self.assertEqual(self.solicitar(), f"<speak>{pista}</speak>")
                    llamada = self.cliente.publish.call_args.kwargs
                    self.assertEqual(llamada["topic"],
                                     "caja-enigmas/caja-enigmas-01/eventos")
                    self.assertEqual(json.loads(llamada["payload"]), {
                        "tipo": "pista", "desafio": desafio, "origen": "voz",
                    })
        self.assertEqual(self.cliente.publish.call_count, 12)
        self.cliente.update_thing_shadow.assert_not_called()

    def test_reinicios_con_catalogo_real(self):
        for cambio in ("sesion", "partida", "desafio", "sesion_nueva"):
            with self.subTest(cambio=cambio):
                self.setUp()
                self.solicitar()
                self.solicitar()
                if cambio == "sesion":
                    self.sesion = "sesion-02"
                elif cambio == "partida":
                    self.reportado["idPartida"] += 1
                elif cambio == "desafio":
                    self.reportado["desafioActual"] = "distancia"
                else:
                    self.es_nueva = True
                desafio = self.reportado["desafioActual"]
                self.assertEqual(self.solicitar(),
                                 f"<speak>{PISTAS_POR_DESAFIO[desafio][0]}</speak>")
                self.assertEqual(self.cliente.publish.call_count, 3)

    def test_consultas_intercaladas_conservan_progreso_y_no_publican(self):
        self.solicitar()
        atributos = copy.deepcopy(self.atributos)
        for intent, texto in (
            ("ConsultarSensoresIntent", "63 centímetros"),
            ("ConsultarProgresoIntent", "en curso"),
            ("ConsultarModoIntent", "modo juego"),
        ):
            with self.subTest(intent=intent):
                self.assertIn(texto, self.solicitar(intent))
                self.assertEqual(self.atributos, atributos)
                self.assertEqual(self.cliente.publish.call_count, 1)
        self.assertEqual(self.solicitar(),
                         f"<speak>{PISTAS_POR_DESAFIO['memoria'][1]}</speak>")
        self.assertEqual(self.cliente.publish.call_count, 2)
        self.cliente.update_thing_shadow.assert_not_called()


if __name__ == "__main__":
    unittest.main()
