"""Operaciones v2 del Shadow con cliente inyectado, sin conexiones AWS."""

from copy import deepcopy
from io import BytesIO
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import MagicMock

from botocore.exceptions import ClientError, EndpointConnectionError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from servicios.servicio_shadow import ServicioShadow


THING = "caja-enigmas-02"


class TestServicioShadowP4(unittest.TestCase):
    """Comprueba parches exactos y que ninguna orden invente confirmaciones."""

    def setUp(self):
        self.cliente = MagicMock()
        self.servicio = ServicioShadow(self.cliente)
        self.cliente.update_thing_shadow.side_effect = (
            lambda **_: {"payload": BytesIO(b'{"state": {}}')}
        )

    def preparar_estado(self, estado):
        self.cliente.get_thing_shadow.side_effect = lambda **_: {
            "payload": BytesIO(json.dumps({"state": estado}).encode())
        }

    def comprobar_parche(self, cambios):
        argumentos = self.cliente.update_thing_shadow.call_args.kwargs
        self.assertEqual(set(argumentos), {"thingName", "payload"})
        self.assertEqual(argumentos["thingName"], THING)
        self.assertIsInstance(argumentos["payload"], bytes)
        self.assertEqual(json.loads(argumentos["payload"]), {
            "state": {"desired": cambios},
        })

    def test_consultas_usan_reported_aunque_desired_difiera(self):
        self.preparar_estado({
            "reported": {"modo": "manual", "dificultad": "facil"},
            "desired": {"modo": "juego", "dificultad": "dificil"},
        })
        self.assertEqual(self.servicio.leer_modo(THING), "manual")
        self.assertEqual(self.servicio.leer_dificultad(THING), "facil")
        self.cliente.update_thing_shadow.assert_not_called()

    def test_consultas_sin_reported_no_usan_desired_ni_defaults(self):
        for estado in ({}, {"desired": {
                "modo": "juego", "dificultad": "normal"}}):
            with self.subTest(estado=estado):
                self.preparar_estado(estado)
                self.assertIsNone(self.servicio.leer_modo(THING))
                self.assertIsNone(self.servicio.leer_dificultad(THING))
                self.assertEqual(self.servicio.leer_parametros(THING), {})

    def test_consulta_parametros_una_lectura_sin_calcular_derivados(self):
        parametros = {
            "dificultad": "normal", "longitudSecuencia": 4,
            "toleranciaCm": 4, "distanciaObjetivoCm": 20,
            "umbralLuzPorcentaje": 70,
        }
        self.preparar_estado({
            "reported": {**parametros, "codigoVinculacion": "0421"},
            "desired": {"dificultad": "dificil"},
        })
        self.assertEqual(self.servicio.leer_parametros(THING), parametros)
        self.cliente.get_thing_shadow.assert_called_once_with(thingName=THING)

    def test_consulta_parametros_parciales(self):
        self.preparar_estado({"reported": {"dificultad": "dificil"}})
        self.assertEqual(self.servicio.leer_parametros(THING), {
            "dificultad": "dificil",
        })

    def test_reportado_invalido_no_se_presenta_como_valido(self):
        casos = (
            ("leer_modo", "modo", None),
            ("leer_dificultad", "dificultad", "difícil"),
            ("leer_parametros", "longitudSecuencia", True),
            ("leer_parametros", "toleranciaCm", 3),
            ("leer_parametros", "distanciaObjetivoCm", 41),
            ("leer_parametros", "umbralLuzPorcentaje", "70"),
        )
        for metodo, campo, valor in casos:
            with self.subTest(campo=campo):
                self.preparar_estado({"reported": {campo: valor}})
                with self.assertRaises(ValueError):
                    getattr(self.servicio, metodo)(THING)

    def test_pedir_juego_limpia_cerradura_en_una_actualizacion(self):
        self.servicio.pedir_modo(THING, "juego")
        self.cliente.update_thing_shadow.assert_called_once()
        self.cliente.get_thing_shadow.assert_not_called()
        self.comprobar_parche({"modo": "juego", "cerradura": None})

    def test_pedir_manual_no_toca_cerradura(self):
        self.servicio.pedir_modo(THING, "manual")
        self.comprobar_parche({"modo": "manual"})

    def test_pedir_dificultades_no_escribe_valores_derivados(self):
        for dificultad in ("facil", "normal", "dificil"):
            with self.subTest(dificultad=dificultad):
                self.servicio.pedir_dificultad(THING, dificultad)
                self.comprobar_parche({"dificultad": dificultad})
        self.cliente.get_thing_shadow.assert_not_called()

    def test_rechaza_modos_y_dificultades_invalidos_antes_de_aws(self):
        for metodo in ("pedir_modo", "pedir_dificultad"):
            for valor in (None, "", "automático", "difícil", 1, True, []):
                with self.subTest(metodo=metodo, valor=valor):
                    with self.assertRaises(ValueError):
                        getattr(self.servicio, metodo)(THING, valor)
        self.cliente.get_thing_shadow.assert_not_called()
        self.cliente.update_thing_shadow.assert_not_called()

    def test_parametros_limites_y_valores_intermedios(self):
        for parametro, valores in (
                ("distanciaObjetivoCm", (10, 20, 40)),
                ("umbralLuzPorcentaje", (10, 70, 95))):
            for valor in valores:
                with self.subTest(parametro=parametro, valor=valor):
                    self.servicio.pedir_parametro(THING, parametro, valor)
                    self.comprobar_parche({parametro: valor})
        self.cliente.get_thing_shadow.assert_not_called()

    def test_parametros_fuera_de_rango_y_tipos_invalidos(self):
        for parametro, valores in (
                ("distanciaObjetivoCm", (9, 41, 20.0, "20", True, None)),
                ("umbralLuzPorcentaje", (9, 96, 70.0, "70", False, {}))):
            for valor in valores:
                with self.subTest(parametro=parametro, valor=valor):
                    with self.assertRaises(ValueError):
                        self.servicio.pedir_parametro(THING, parametro, valor)
        self.cliente.update_thing_shadow.assert_not_called()

    def test_no_permite_parametros_de_solo_lectura(self):
        for parametro in ("longitudSecuencia", "toleranciaCm", "modo",
                          "solicitudPartida", "tapa", None, []):
            with self.subTest(parametro=parametro):
                with self.assertRaises(ValueError):
                    self.servicio.pedir_parametro(THING, parametro, 4)
        self.cliente.update_thing_shadow.assert_not_called()

    def test_partida_incrementa_reported_nunca_desired(self):
        for deseado in (None, 0, 8, 900, "inválido"):
            with self.subTest(deseado=deseado):
                self.preparar_estado({
                    "reported": {"modo": "juego", "solicitudPartida": 7},
                    "desired": {"solicitudPartida": deseado},
                })
                self.servicio.pedir_partida(THING)
                self.comprobar_parche({"solicitudPartida": 8})

    def test_contador_cero_y_sin_desired(self):
        self.preparar_estado({"reported": {
            "modo": "juego", "solicitudPartida": 0,
        }})
        self.servicio.pedir_partida(THING)
        self.comprobar_parche({"solicitudPartida": 1})
        self.cliente.get_thing_shadow.assert_called_once_with(thingName=THING)

    def test_orden_repetida_sin_confirmacion_repite_el_mismo_contador(self):
        self.preparar_estado({"reported": {
            "modo": "juego", "solicitudPartida": 7,
        }, "desired": {"solicitudPartida": 8}})
        self.servicio.pedir_partida(THING)
        self.servicio.pedir_partida(THING)
        for llamada in self.cliente.update_thing_shadow.call_args_list:
            self.assertEqual(json.loads(llamada.kwargs["payload"]), {
                "state": {"desired": {"solicitudPartida": 8}},
            })

    def test_partida_con_contador_invalido_o_ausente_no_escribe(self):
        reportes = [{"modo": "juego"}] + [
            {"modo": "juego", "solicitudPartida": valor}
            for valor in (None, -1, True, "0", 0.0)
        ]
        for reportado in reportes:
            with self.subTest(reportado=reportado):
                self.preparar_estado({"reported": reportado,
                                     "desired": {"solicitudPartida": 99}})
                with self.assertRaises(ValueError):
                    self.servicio.pedir_partida(THING)
        self.cliente.update_thing_shadow.assert_not_called()

    def test_partida_requiere_modo_juego_reportado(self):
        for modo in (None, "manual", "desconocido"):
            self.preparar_estado({
                "reported": {"modo": modo, "solicitudPartida": 7},
                "desired": {"modo": "juego"},
            })
            with self.assertRaises(ValueError):
                self.servicio.pedir_partida(THING)
        self.preparar_estado({})
        with self.assertRaises(ValueError):
            self.servicio.pedir_partida(THING)
        self.cliente.update_thing_shadow.assert_not_called()

    def test_parche_conserva_otros_desired_y_reported_en_cliente_falso(self):
        estado = {
            "reported": {"modo": "manual", "codigoVinculacion": "0421"},
            "desired": {"cerradura": "abierta", "dificultad": "dificil",
                        "distanciaObjetivoCm": 30},
        }
        reportado_original = deepcopy(estado["reported"])

        def aplicar_parche(**argumentos):
            parche = json.loads(argumentos["payload"])
            self.assertEqual(set(parche["state"]), {"desired"})
            for campo, valor in parche["state"]["desired"].items():
                if valor is None:
                    estado["desired"].pop(campo, None)
                else:
                    estado["desired"][campo] = valor
            return {"payload": BytesIO(json.dumps({"state": estado}).encode())}

        self.cliente.update_thing_shadow.side_effect = aplicar_parche
        respuesta = self.servicio.pedir_modo(THING, "juego")
        self.assertEqual(estado["desired"], {
            "modo": "juego", "dificultad": "dificil",
            "distanciaObjetivoCm": 30,
        })
        self.assertEqual(respuesta["state"]["reported"], reportado_original)
        self.assertEqual(estado["reported"], reportado_original)

    def test_errores_de_lectura_aws_no_generan_escrituras(self):
        errores = (ClientError({"Error": {
            "Code": "ResourceNotFoundException", "Message": "Sin Shadow",
        }}, "GetThingShadow"), EndpointConnectionError(
            endpoint_url="https://ejemplo.invalid"
        ))
        for error in errores:
            for metodo in ("leer_modo", "leer_dificultad", "leer_parametros",
                           "pedir_partida"):
                self.cliente.get_thing_shadow.side_effect = error
                with self.assertRaises(type(error)) as capturado:
                    getattr(self.servicio, metodo)(THING)
                self.assertIs(capturado.exception, error)
        self.cliente.update_thing_shadow.assert_not_called()

    def test_errores_de_escritura_no_se_convierten_en_exito(self):
        error = ClientError({"Error": {
            "Code": "UnauthorizedException", "Message": "Denegado",
        }}, "UpdateThingShadow")
        self.cliente.update_thing_shadow.side_effect = error
        self.preparar_estado({"reported": {
            "modo": "juego", "solicitudPartida": 1,
        }})
        llamadas = (
            ("pedir_modo", "juego"), ("pedir_dificultad", "normal"),
            ("pedir_parametro", "distanciaObjetivoCm", 20),
            ("pedir_partida",),
        )
        for metodo, *argumentos in llamadas:
            with self.subTest(metodo=metodo):
                with self.assertRaises(ClientError) as capturado:
                    getattr(self.servicio, metodo)(THING, *argumentos)
                self.assertIs(capturado.exception, error)

    def test_respuesta_de_actualizacion_invalida_cierra_stream(self):
        payload = BytesIO(b"JSON invalido")
        self.cliente.update_thing_shadow.side_effect = None
        self.cliente.update_thing_shadow.return_value = {"payload": payload}
        with self.assertRaises(json.JSONDecodeError):
            self.servicio.pedir_modo(THING, "manual")
        self.assertTrue(payload.closed)


if __name__ == "__main__":
    unittest.main()
