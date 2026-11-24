"""RF4 por el dispatcher real, con vinculación y Shadow simulados."""

import json
import unittest
from io import BytesIO
from unittest.mock import MagicMock, Mock

from ask_sdk_core.handler_input import HandlerInput
from ask_sdk_core.serialize import DefaultSerializer
from ask_sdk_model import RequestEnvelope
from botocore.exceptions import ClientError

from apoyo import crear_lambda, evento_alexa, voz
from manejadores.consultar_sensores import ConsultarSensoresIntentHandler
from servicios.repositorio_vinculaciones import RepositorioVinculaciones
from servicios.servicio_shadow import ServicioShadow


class TestConsultarSensores(unittest.TestCase):
    """Verifica lecturas reales, aislamiento por usuario y contrato v2."""

    def setUp(self):
        self.repositorio = MagicMock(spec=RepositorioVinculaciones)
        self.repositorio.obtener.return_value = {
            "thingName": "caja-enigmas-02",
        }
        self.reportado = {"distanciaCm": 63, "luzPorcentaje": 22}
        self.cliente = Mock()
        self.cliente.get_thing_shadow.side_effect = self.leer_shadow
        self.shadow = ServicioShadow(self.cliente)
        self.invocar = crear_lambda(self.repositorio, Mock(), self.shadow)

    def leer_shadow(self, **argumentos):
        return {"payload": BytesIO(json.dumps({"state": {
            "reported": self.reportado,
            "desired": {"distanciaCm": 99, "luzPorcentaje": 88},
        }}).encode())}

    def solicitar(self, usuario="usuario-02"):
        respuesta = self.invocar(
            evento_alexa("ConsultarSensoresIntent", usuario), None
        )
        self.cliente.update_thing_shadow.assert_not_called()
        self.cliente.publish.assert_not_called()
        self.repositorio.guardar.assert_not_called()
        self.repositorio.eliminar.assert_not_called()
        self.assertFalse(respuesta["response"]["shouldEndSession"])
        return voz(respuesta)

    def test_registro_en_skill_y_ambas_lecturas_de_caja_vinculada(self):
        mensaje = self.solicitar()
        self.assertIn("63 centímetros", mensaje)
        self.assertIn("22 por ciento", mensaje)
        self.repositorio.obtener.assert_called_once_with("usuario-02")
        self.cliente.get_thing_shadow.assert_called_once_with(
            thingName="caja-enigmas-02"
        )

    def test_solo_distancia(self):
        self.reportado = {"distanciaCm": 63}
        mensaje = self.solicitar()
        self.assertIn("63 centímetros", mensaje)
        self.assertIn("No tengo una lectura válida de luz", mensaje)

    def test_solo_luz(self):
        self.reportado = {"luzPorcentaje": 22}
        mensaje = self.solicitar()
        self.assertIn("22 por ciento", mensaje)
        self.assertIn("No tengo una lectura válida de distancia", mensaje)

    def test_sin_lecturas_no_usa_desired_ni_objetivos(self):
        self.reportado = {"distanciaObjetivoCm": 20,
                          "umbralLuzPorcentaje": 70}
        mensaje = self.solicitar()
        self.assertIn("No tengo lecturas válidas de distancia ni de luz", mensaje)
        for valor in ("99", "88", "20", "70"):
            self.assertNotIn(valor, mensaje)

    def test_luz_cero_es_valida(self):
        self.reportado["luzPorcentaje"] = 0
        self.assertIn("0 por ciento", self.solicitar())

    def test_limites_validos(self):
        for distancia, luz in ((2, 0), (400, 100)):
            with self.subTest(distancia=distancia, luz=luz):
                self.reportado = {"distanciaCm": distancia,
                                  "luzPorcentaje": luz}
                mensaje = self.solicitar()
                self.assertIn(f"{distancia} centímetros", mensaje)
                self.assertIn(f"{luz} por ciento", mensaje)

    def test_valores_invalidos_no_se_anuncian_como_mediciones(self):
        for campo, valores, falta in (
            ("distanciaCm", (0, 1, 401, -1, True, 2.5, "63", None),
             "distancia"),
            ("luzPorcentaje", (-1, 101, False, 22.5, "22", None), "luz"),
        ):
            for valor in valores:
                with self.subTest(campo=campo, valor=valor):
                    self.reportado = {"distanciaCm": 63, "luzPorcentaje": 22}
                    self.reportado[campo] = valor
                    self.assertIn(
                        f"No tengo una lectura válida de {falta}",
                        self.solicitar(),
                    )

    def test_sin_vinculacion(self):
        self.repositorio.obtener.return_value = None
        self.assertIn("vincula", self.solicitar().lower())
        self.cliente.get_thing_shadow.assert_not_called()

    def test_sin_usuario_no_usa_identidad_de_sesion(self):
        self.assertIn("usuario", self.solicitar(usuario=None).lower())
        self.repositorio.obtener.assert_not_called()
        self.cliente.get_thing_shadow.assert_not_called()

    def test_error_shadow_utiliza_manejador_global(self):
        self.cliente.get_thing_shadow.side_effect = ClientError({
            "Error": {"Code": "AccessDeniedException", "Message": "Prueba"},
        }, "GetThingShadow")
        with self.assertLogs("manejadores.errores", level="ERROR"):
            self.assertIn("No pude completar", self.solicitar())

    def test_can_handle_solo_intent_de_sensores(self):
        manejador = ConsultarSensoresIntentHandler(self.repositorio, self.shadow)
        for intent, tipo, esperado in (
            ("ConsultarSensoresIntent", "IntentRequest", True),
            ("ConsultarParametrosIntent", "IntentRequest", False),
            ("ConsultarCerraduraIntent", "IntentRequest", False),
            (None, "LaunchRequest", False),
        ):
            with self.subTest(intent=intent, tipo=tipo):
                sobre = DefaultSerializer().deserialize(
                    json.dumps(evento_alexa(intent, tipo=tipo)), RequestEnvelope
                )
                entrada = HandlerInput(request_envelope=sobre)
                self.assertEqual(manejador.can_handle(entrada), esperado)


if __name__ == "__main__":
    unittest.main()
