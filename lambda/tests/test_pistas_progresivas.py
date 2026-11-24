"""Secuencia de pistas con atributos devueltos entre peticiones de Alexa."""

import copy
import json
import unittest
from unittest.mock import Mock, patch, PropertyMock, call

from ask_sdk_core.attributes_manager import AttributesManager
from apoyo import crear_lambda, evento_alexa, voz
from servicios.publicador_eventos import PublicadorEventos


class TestPistasProgresivas(unittest.TestCase):
    """Usa contenido simulado sin suponer la estructura del catálogo real."""

    def setUp(self):
        self.repo = Mock()
        self.repo.obtener.return_value = {"thingName": "caja-enigmas-01"}
        self.shadow = Mock()
        self.reportado = {"estadoPartida": "en_curso", "idPartida": 12,
                          "desafioActual": "memoria"}
        self.shadow.leer_reported.return_value = self.reportado
        self.publicador = Mock()
        self.proveedor = Mock(side_effect=lambda desafio, nivel:
                              f"PRUEBA {desafio} nivel {nivel}")
        self.invocar = crear_lambda(
            self.repo, Mock(), self.shadow,
            publicador_eventos=self.publicador,
            obtener_pista_progresiva=self.proveedor,
        )
        self.atributos = {}
        self.sesion = "sesion-01"
        self.usuario = "usuario-01"
        self.es_nueva = True

    def solicitar(self, intent="PedirPistaIntent"):
        evento = evento_alexa(intent, self.usuario)
        evento["session"].update({"sessionId": self.sesion,
                                  "new": self.es_nueva,
                                  "attributes": copy.deepcopy(self.atributos)})
        respuesta = self.invocar(evento, None)
        # Simula el transporte JSON de Alexa; no comparte objetos del SDK.
        self.assertIn("sessionAttributes", respuesta)
        self.ultima_respuesta = json.loads(json.dumps(respuesta))
        self.atributos = self.ultima_respuesta["sessionAttributes"]
        self.es_nueva = False
        return voz(respuesta)

    def test_tres_niveles_y_repeticion_publican_cada_vez(self):
        for nivel in (1, 2, 3, 3, 3):
            self.assertIn(f"nivel {nivel}", self.solicitar())
        self.assertEqual(self.proveedor.call_args_list,
                         [call("memoria", nivel)
                          for nivel in (1, 2, 3, 3, 3)])
        self.assertEqual(self.publicador.publicar_pista.call_count, 5)
        self.publicador.publicar_pista.assert_called_with(
            "caja-enigmas-01", "memoria"
        )

    def test_atributos_salen_en_respuesta_y_se_recuperan_de_peticion(self):
        self.solicitar()
        progreso = self.ultima_respuesta["sessionAttributes"]["progresoPistas"]
        self.assertEqual(progreso, {
            "contexto": {"usuario": "usuario-01", "caja": "caja-enigmas-01",
                         "sesion": "sesion-01", "partida": 12,
                         "desafio": "memoria"},
            "nivel": 1,
        })
        self.assertIn("nivel 2", self.solicitar())
        self.assertEqual(self.atributos["progresoPistas"]["nivel"], 2)
        self.assertEqual(progreso["nivel"], 1)
        # Sin reenviar atributos el SDK no recupera estado por sessionId.
        self.atributos = {}
        self.assertIn("nivel 1", self.solicitar())

    def test_cada_cambio_de_contexto_reinicia_desde_nivel_avanzado(self):
        for campo in ("usuario", "caja", "sesion", "partida", "desafio"):
            with self.subTest(campo=campo):
                self.setUp()
                self.solicitar()
                self.solicitar()
                self.assertEqual(self.atributos["progresoPistas"]["nivel"], 2)
                if campo == "usuario":
                    self.usuario = "usuario-02"
                elif campo == "caja":
                    self.repo.obtener.return_value = {
                        "thingName": "caja-enigmas-02",
                    }
                elif campo == "sesion":
                    self.sesion = "sesion-02"
                elif campo == "partida":
                    self.reportado["idPartida"] = 13
                else:
                    self.reportado["desafioActual"] = "distancia"
                self.assertIn("nivel 1", self.solicitar())
                self.assertEqual(self.atributos["progresoPistas"]["nivel"], 1)

    def test_contenido_invalido_conserva_contador_y_no_publica(self):
        self.solicitar()
        anterior = copy.deepcopy(self.atributos)
        self.publicador.reset_mock()
        self.proveedor.side_effect = None
        for contenido in (None, "", " \n\t ", False, 0, 3, [], {}):
            with self.subTest(contenido=contenido):
                self.proveedor.return_value = contenido
                self.assertIn("No hay una pista", self.solicitar())
                self.assertEqual(self.atributos, anterior)
                self.publicador.publicar_pista.assert_not_called()
                self.proveedor.assert_called_with("memoria", 2)

    def test_fallo_publicacion_restaura_contador_existente(self):
        self.solicitar()
        anterior = copy.deepcopy(self.atributos)
        self.publicador.publicar_pista.side_effect = RuntimeError("simulado")
        with self.assertLogs("manejadores.errores", level="ERROR"):
            self.assertNotIn("PRUEBA", self.solicitar())
        self.assertEqual(self.atributos, anterior)
        self.publicador.publicar_pista.side_effect = None
        self.assertIn("nivel 2", self.solicitar())
        self.assertEqual(self.publicador.publicar_pista.call_count, 3)

    def test_proveedor_anterior_conserva_firma_y_no_crea_contador(self):
        proveedor_anterior = Mock(return_value="PRUEBA <anterior> & texto")
        self.invocar = crear_lambda(
            self.repo, Mock(), self.shadow,
            publicador_eventos=self.publicador,
            obtener_pista=proveedor_anterior,
        )
        self.reportado.pop("idPartida")
        for cantidad in (1, 2):
            self.assertIn("PRUEBA &lt;anterior&gt; &amp; texto", self.solicitar())
            self.assertEqual(proveedor_anterior.call_args, call("memoria"))
            self.assertEqual(proveedor_anterior.call_count, cantidad)
            self.assertEqual(self.publicador.publicar_pista.call_count, cantidad)
            self.assertNotIn("progresoPistas", self.atributos)

    def test_cambio_desafio_reinicia_incluso_al_volver(self):
        self.solicitar()
        for desafio in ("distancia", "memoria", "luz"):
            self.reportado["desafioActual"] = desafio
            self.assertIn(f"{desafio} nivel 1", self.solicitar())

    def test_nueva_partida_reportada_reinicia(self):
        self.solicitar()
        self.reportado["idPartida"] += 1
        self.assertIn("nivel 1", self.solicitar())

    def test_nueva_sesion_reinicia_aun_con_atributos_anteriores(self):
        self.solicitar()
        self.sesion = "sesion-02"
        self.assertIn("nivel 1", self.solicitar())
        self.es_nueva = True
        self.assertIn("nivel 1", self.solicitar())

    def test_aislamiento_usuario_y_caja(self):
        self.solicitar()
        self.usuario = "usuario-02"
        self.assertIn("nivel 1", self.solicitar())
        self.repo.obtener.return_value = {"thingName": "caja-enigmas-02"}
        self.assertIn("nivel 1", self.solicitar())
        self.publicador.publicar_pista.assert_called_with(
            "caja-enigmas-02", "memoria"
        )

    def test_sin_catalogo_no_publica_ni_avanza(self):
        self.proveedor.side_effect = None
        self.proveedor.return_value = None
        self.assertIn("No hay una pista", self.solicitar())
        self.publicador.publicar_pista.assert_not_called()
        self.assertEqual(self.atributos, {})
        self.proveedor.return_value = "PRUEBA"
        self.solicitar()
        self.proveedor.assert_called_with("memoria", 1)

    def test_id_partida_invalido_no_entrega(self):
        for valor in (None, -1, True, "12", 1.5):
            self.reportado["idPartida"] = valor
            self.assertIn("No tengo", self.solicitar())
        self.proveedor.assert_not_called()
        self.publicador.publicar_pista.assert_not_called()

    def test_fallo_lectura_no_publica_ni_avanza(self):
        self.solicitar()
        atributos = copy.deepcopy(self.atributos)
        self.shadow.leer_reported.side_effect = RuntimeError("simulado")
        with self.assertLogs("manejadores.errores", level="ERROR"):
            self.assertIn("No pude completar", self.solicitar())
        self.assertEqual(self.atributos, atributos)
        self.assertEqual(self.publicador.publicar_pista.call_count, 1)

    def test_fallo_publicacion_no_entrega_ni_avanza(self):
        self.publicador.publicar_pista.side_effect = RuntimeError("simulado")
        with self.assertLogs("manejadores.errores", level="ERROR"):
            self.assertNotIn("PRUEBA", self.solicitar())
        self.assertEqual(self.atributos, {})
        self.publicador.publicar_pista.side_effect = None
        self.assertIn("nivel 1", self.solicitar())

    def test_fallo_guardado_sesion_no_publica(self):
        with patch.object(AttributesManager, "session_attributes",
                          new_callable=PropertyMock) as propiedad:
            propiedad.side_effect = [{}, RuntimeError("simulado"), {}]
            with self.assertLogs("manejadores.errores", level="ERROR"):
                self.assertIn("No pude completar", self.solicitar())
        self.publicador.publicar_pista.assert_not_called()

    def test_fallo_lectura_sesion_no_publica(self):
        with patch.object(AttributesManager, "session_attributes",
                          new_callable=PropertyMock) as propiedad:
            propiedad.side_effect = [RuntimeError("simulado"), {}]
            with self.assertLogs("manejadores.errores", level="ERROR"):
                self.assertIn("No pude completar", self.solicitar())
        self.publicador.publicar_pista.assert_not_called()

    def test_otros_intents_conservan_el_contador(self):
        self.solicitar()
        self.solicitar("ConsultarProgresoIntent")
        self.assertIn("nivel 2", self.solicitar())

    def test_estado_corrupto_no_publica(self):
        self.solicitar()
        self.atributos["progresoPistas"]["nivel"] = True
        with self.assertLogs("manejadores.errores", level="ERROR"):
            self.assertIn("No pude completar", self.solicitar())
        self.assertEqual(self.publicador.publicar_pista.call_count, 1)

    def test_fallo_proveedor_no_publica_ni_avanza(self):
        self.proveedor.side_effect = RuntimeError("simulado")
        with self.assertLogs("manejadores.errores", level="ERROR"):
            self.assertIn("No pude completar", self.solicitar())
        self.assertEqual(self.atributos, {})
        self.publicador.publicar_pista.assert_not_called()

    def test_estado_no_depende_de_instancia_lambda(self):
        self.solicitar()
        self.invocar = crear_lambda(
            self.repo, Mock(), self.shadow,
            publicador_eventos=self.publicador,
            obtener_pista_progresiva=self.proveedor,
        )
        self.assertIn("nivel 2", self.solicitar())

    def test_eventos_reales_conservan_payload_del_contrato(self):
        cliente = Mock()
        self.invocar = crear_lambda(
            self.repo, Mock(), self.shadow,
            publicador_eventos=PublicadorEventos(cliente),
            obtener_pista_progresiva=self.proveedor,
        )
        self.atributos = {"otroIntent": "conservar"}
        for nivel in (1, 2, 3, 3):
            self.assertIn(f"nivel {nivel}", self.solicitar())
        self.assertEqual(cliente.publish.call_count, 4)
        for llamada in cliente.publish.call_args_list:
            self.assertEqual(llamada.kwargs["topic"],
                             "caja-enigmas/caja-enigmas-01/eventos")
            self.assertEqual(json.loads(llamada.kwargs["payload"]), {
                "tipo": "pista", "desafio": "memoria", "origen": "voz",
            })
        self.assertEqual(self.atributos["otroIntent"], "conservar")


if __name__ == "__main__":
    unittest.main()
