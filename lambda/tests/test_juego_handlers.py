"""Intents P4 por el dispatcher del SDK con Shadow y MQTT simulados."""

from io import BytesIO
import json
import unittest
from unittest.mock import MagicMock, Mock
from xml.etree import ElementTree

from botocore.exceptions import ClientError, EndpointConnectionError

from apoyo import crear_lambda, evento_alexa, voz
from servicios.publicador_eventos import PublicadorEventos
from servicios.repositorio_vinculaciones import RepositorioVinculaciones
from servicios.servicio_registro import ServicioRegistro
from servicios.servicio_shadow import ServicioShadow


INTENTS = (
    "IniciarPartidaIntent", "PedirPistaIntent", "ConsultarProgresoIntent",
    "CambiarModoIntent", "ConsultarModoIntent", "CambiarDificultadIntent",
    "CambiarParametroIntent", "ConsultarParametrosIntent",
)


def slot_resuelto(nombre, identificador, texto="sinónimo de prueba"):
    return {
        "name": nombre, "value": texto,
        "resolutions": {"resolutionsPerAuthority": [{
            "authority": "autoridad-prueba",
            "status": {"code": "ER_SUCCESS_MATCH"},
            "values": [{"value": {"name": texto, "id": identificador}}],
        }]},
    }


class TestJuegoHandlers(unittest.TestCase):
    """Prueba identidad, parches, rechazos y pistas sin módulo de Lucía."""

    def setUp(self):
        self.repo = MagicMock(spec=RepositorioVinculaciones)
        self.repo.obtener.side_effect = lambda usuario: {
            "thingName": "caja-enigmas-02" if usuario == "usuario-02"
            else "caja-enigmas-01",
        }
        self.registro = MagicMock(spec=ServicioRegistro)
        self.estado = {
            "reported": {
                "modo": "juego", "solicitudPartida": 7,
                "estadoPartida": "en_curso", "desafioActual": "memoria",
                "dificultad": "normal", "longitudSecuencia": 4,
                "toleranciaCm": 4, "distanciaObjetivoCm": 20,
                "umbralLuzPorcentaje": 70, "cerradura": "cerrada",
            },
            "desired": {"solicitudPartida": 100, "modo": "manual",
                        "dificultad": "dificil"},
        }
        self.iot = Mock()
        self.iot.get_thing_shadow.side_effect = self.leer_shadow
        self.iot.update_thing_shadow.side_effect = self.actualizar_shadow
        self.shadow = ServicioShadow(self.iot)
        self.proveedor = Mock(return_value="CONTENIDO SOLO DE PRUEBA")
        self.publicador = PublicadorEventos(self.iot)
        self.invocar = crear_lambda(
            self.repo, self.registro, self.shadow,
            publicador_eventos=self.publicador, obtener_pista=self.proveedor,
        )

    def leer_shadow(self, **argumentos):
        return {"payload": BytesIO(json.dumps({
            "state": self.estado,
        }).encode())}

    def actualizar_shadow(self, **argumentos):
        parche = json.loads(argumentos["payload"])
        self.assertEqual(set(parche["state"]), {"desired"})
        for campo, valor in parche["state"]["desired"].items():
            if valor is None:
                self.estado["desired"].pop(campo, None)
            else:
                self.estado["desired"][campo] = valor
        return self.leer_shadow()

    def solicitar(self, intent, slots=None, usuario="usuario-01"):
        evento = evento_alexa(intent, usuario)
        evento["request"]["intent"]["slots"] = slots or {}
        return self.invocar(evento, None)

    def comprobar_sin_escritura(self):
        self.iot.update_thing_shadow.assert_not_called()
        self.iot.publish.assert_not_called()

    def test_todos_los_intents_sin_identidad_no_consultan_servicios(self):
        for intent in INTENTS:
            mensaje = voz(self.solicitar(intent, usuario=None))
            self.assertIn("No pude identificar", mensaje)
        self.repo.obtener.assert_not_called()
        self.iot.get_thing_shadow.assert_not_called()
        self.comprobar_sin_escritura()

    def test_todos_los_intents_sin_vinculacion_explican_como_vincular(self):
        self.repo.obtener.side_effect = None
        self.repo.obtener.return_value = None
        for intent in INTENTS:
            mensaje = voz(self.solicitar(intent))
            self.assertIn("no tienes una caja vinculada", mensaje)
            self.assertIn("vincula mi caja", mensaje)
        self.iot.get_thing_shadow.assert_not_called()
        self.comprobar_sin_escritura()

    def test_inicio_usa_reported_mas_uno_y_no_confirma_inicio(self):
        mensaje = voz(self.solicitar("IniciarPartidaIntent"))
        self.assertEqual(self.estado["desired"]["solicitudPartida"], 8)
        self.assertEqual(self.estado["reported"]["solicitudPartida"], 7)
        self.assertIn("pendiente de confirmación", mensaje)
        self.assertNotIn("partida iniciada", mensaje)
        self.solicitar("IniciarPartidaIntent")
        self.assertEqual(self.estado["desired"]["solicitudPartida"], 8)

    def test_inicio_en_manual_es_rechazado_aunque_desired_sea_juego(self):
        self.estado["reported"]["modo"] = "manual"
        self.estado["desired"]["modo"] = "juego"
        mensaje = voz(self.solicitar("IniciarPartidaIntent"))
        self.assertIn("modo manual", mensaje)
        self.comprobar_sin_escritura()

    def test_inicio_con_shadow_incompleto_no_escribe(self):
        for reportado in ({}, {"modo": "juego"},
                          {"modo": "juego", "solicitudPartida": True}):
            self.estado["reported"] = reportado
            mensaje = voz(self.solicitar("IniciarPartidaIntent"))
            self.assertIn("reporte", mensaje)
        self.comprobar_sin_escritura()

    def test_cambiar_modo_usa_id_y_limpia_cerradura(self):
        self.estado["desired"]["cerradura"] = "abierta"
        slots = {"modo": slot_resuelto("modo", "juego", "automático")}
        mensaje = voz(self.solicitar("CambiarModoIntent", slots))
        argumentos = self.iot.update_thing_shadow.call_args.kwargs
        parche = json.loads(argumentos["payload"])
        self.assertEqual(parche, {"state": {
            "desired": {"modo": "juego", "cerradura": None},
        }})
        self.assertIn("pendiente de confirmación", mensaje)

    def test_modo_dificultad_y_parametro_sin_resolucion_no_usan_texto(self):
        for intent, nombre, texto in (
            ("CambiarModoIntent", "modo", "juego"),
            ("CambiarDificultadIntent", "dificultad", "facil"),
            ("CambiarParametroIntent", "parametro", "distanciaObjetivoCm"),
        ):
            self.solicitar(intent, {nombre: {"name": nombre, "value": texto}})
        self.comprobar_sin_escritura()

    def test_resoluciones_fallidas_invalidas_o_ambiguas_no_escriben(self):
        for codigo in ("ER_SUCCESS_NO_MATCH", "ER_ERROR_TIMEOUT"):
            slot = slot_resuelto("modo", "manual")
            autoridad = slot["resolutions"]["resolutionsPerAuthority"][0]
            autoridad["status"]["code"] = codigo
            self.solicitar("CambiarModoIntent", {"modo": slot})
        slot = slot_resuelto("modo", "otro")
        self.solicitar("CambiarModoIntent", {"modo": slot})
        slot = slot_resuelto("modo", "manual")
        slot["resolutions"]["resolutionsPerAuthority"][0]["values"].append(
            {"value": {"name": "juego", "id": "juego"}}
        )
        self.solicitar("CambiarModoIntent", {"modo": slot})
        self.comprobar_sin_escritura()

    def test_consultar_modo_no_usa_desired(self):
        self.assertIn("modo juego", voz(self.solicitar("ConsultarModoIntent")))
        self.estado["reported"] = {}
        self.assertIn("No tengo información reportada", voz(
            self.solicitar("ConsultarModoIntent")))
        self.comprobar_sin_escritura()

    def test_cambiar_dificultad_solo_modifica_dificultad(self):
        for identificador in ("facil", "normal", "dificil"):
            respuesta = self.solicitar("CambiarDificultadIntent", {
                "dificultad": slot_resuelto("dificultad", identificador),
            })
            self.assertIn("pendiente", voz(respuesta))
            argumentos = self.iot.update_thing_shadow.call_args.kwargs
            parche = json.loads(argumentos["payload"])
            self.assertEqual(parche["state"]["desired"], {
                "dificultad": identificador,
            })
        self.assertEqual(self.estado["reported"]["dificultad"], "normal")

    def test_cambiar_parametros_en_limites(self):
        for parametro, valores in (("distanciaObjetivoCm", (10, 40)),
                                  ("umbralLuzPorcentaje", (10, 95))):
            for valor in valores:
                self.solicitar("CambiarParametroIntent", {
                    "parametro": slot_resuelto("parametro", parametro),
                    "valor": {"name": "valor", "value": str(valor)},
                })
                self.assertEqual(self.estado["desired"][parametro], valor)
                self.assertIs(type(self.estado["desired"][parametro]), int)

    def test_parametros_invalidos_explican_rango_sin_escribir(self):
        for parametro, valores, maximo in (
            ("distanciaObjetivoCm", ("9", "41", "20.5", "true", None), 40),
            ("umbralLuzPorcentaje", ("9", "96", "", "veinte"), 95),
        ):
            for valor in valores:
                mensaje = voz(self.solicitar("CambiarParametroIntent", {
                    "parametro": slot_resuelto("parametro", parametro),
                    "valor": {"name": "valor", "value": valor},
                }))
                self.assertIn(f"entre 10 y {maximo}", mensaje)
        self.comprobar_sin_escritura()

    def test_consultar_parametros_reportados_e_incompletos(self):
        mensaje = voz(self.solicitar("ConsultarParametrosIntent"))
        self.assertIn("dificultad normal", mensaje)
        self.assertIn("centímetros: 20", mensaje)
        self.assertNotIn("difícil", mensaje)
        self.estado["reported"] = {"dificultad": "facil"}
        mensaje = voz(self.solicitar("ConsultarParametrosIntent"))
        self.assertIn("incompleto", mensaje)
        self.assertIn("fácil", mensaje)
        self.assertNotIn("secuencia", mensaje)
        self.estado["reported"] = {}
        self.assertIn("No tengo parámetros", voz(
            self.solicitar("ConsultarParametrosIntent")))
        self.comprobar_sin_escritura()

    def test_progreso_reportado_y_no_deseado(self):
        for estado, desafio, esperado in (
            ("esperando", "ninguno", "esperando una partida"),
            ("resuelta", "ninguno", "partida está resuelta"),
            ("en_curso", "distancia", "desafío de distancia"),
            ("en_curso", "ninguno", "No tengo un reporte completo"),
        ):
            self.estado["reported"].update(estadoPartida=estado,
                                           desafioActual=desafio)
            mensaje = voz(self.solicitar("ConsultarProgresoIntent"))
            self.assertIn(esperado, mensaje)
        self.estado["reported"] = {}
        mensaje = voz(self.solicitar("ConsultarProgresoIntent"))
        self.assertIn("No tengo", mensaje)
        self.comprobar_sin_escritura()

    def test_pistas_publican_solo_los_campos_del_contrato(self):
        for desafio in ("memoria", "distancia", "luz"):
            self.estado["reported"]["desafioActual"] = desafio
            respuesta = self.solicitar("PedirPistaIntent", usuario="usuario-02")
            mensaje = voz(respuesta)
            self.assertIn("CONTENIDO SOLO DE PRUEBA", mensaje)
            argumentos = self.iot.publish.call_args.kwargs
            self.assertEqual(argumentos["topic"],
                             "caja-enigmas/caja-enigmas-02/eventos")
            self.assertEqual(argumentos["qos"], 1)
            self.assertFalse(argumentos["retain"])
            self.assertEqual(json.loads(argumentos["payload"]), {
                "tipo": "pista", "desafio": desafio, "origen": "voz",
            })
            self.proveedor.assert_called_with(desafio)
        self.iot.update_thing_shadow.assert_not_called()

    def test_pistas_sin_partida_o_reporte_valido_no_publican(self):
        for estado, desafio in (("esperando", "ninguno"),
                                ("resuelta", "ninguno"),
                                ("en_curso", "ninguno"), (None, None)):
            self.estado["reported"].update(estadoPartida=estado,
                                           desafioActual=desafio)
            self.solicitar("PedirPistaIntent")
        self.proveedor.assert_not_called()
        self.comprobar_sin_escritura()

    def test_pistas_pendientes_de_lucia_no_se_inventan(self):
        self.invocar = crear_lambda(self.repo, self.registro, self.shadow,
                                   publicador_eventos=self.publicador)
        self.assertIn("todavía no están disponibles", voz(
            self.solicitar("PedirPistaIntent")))
        self.proveedor.assert_not_called()
        self.comprobar_sin_escritura()

    def test_contenido_vacio_no_publica_y_ssml_se_escapa(self):
        for contenido in (None, "", "   "):
            self.proveedor.return_value = contenido
            self.solicitar("PedirPistaIntent")
        self.iot.publish.assert_not_called()
        self.proveedor.return_value = "Texto <dato>de prueba</dato> & prueba"
        mensaje = voz(self.solicitar("PedirPistaIntent"))
        raiz = ElementTree.fromstring(mensaje)
        self.assertIsNone(raiz.find("dato"))
        self.assertIn("&amp;", mensaje)

    def test_fallo_publicacion_no_entrega_pista_como_exito(self):
        self.iot.publish.side_effect = ClientError({"Error": {
            "Code": "UnauthorizedException", "Message": "DATO_PRIVADO",
        }}, "Publish")
        with self.assertLogs("manejadores.errores", level="ERROR") as logs:
            mensaje = voz(self.solicitar("PedirPistaIntent"))
        self.assertIn("No pude completar", mensaje)
        self.assertNotIn("CONTENIDO SOLO DE PRUEBA", mensaje)
        self.assertNotIn("DATO_PRIVADO", str(logs.output))

    def test_cerradura_juego_no_escribe_aunque_manual_este_pendiente(self):
        for intent in ("AbrirCerraduraIntent", "CerrarCerraduraIntent"):
            self.assertIn("modo juego", voz(self.solicitar(intent)))
        self.comprobar_sin_escritura()

    def test_cerradura_sin_modo_reportado_no_asume_manual(self):
        self.estado["reported"].pop("modo")
        self.assertIn("No tengo un modo manual", voz(
            self.solicitar("AbrirCerraduraIntent")))
        self.comprobar_sin_escritura()

    def test_cerradura_manual_solo_envia_orden(self):
        self.estado["reported"]["modo"] = "manual"
        self.estado["desired"]["modo"] = "juego"
        mensaje = voz(self.solicitar("AbrirCerraduraIntent"))
        self.assertIn("Envié la orden", mensaje)
        self.assertEqual(self.estado["reported"]["cerradura"], "cerrada")
        self.assertEqual(self.estado["desired"]["cerradura"], "abierta")

    def test_error_lectura_o_escritura_no_anuncia_exito(self):
        self.iot.get_thing_shadow.side_effect = EndpointConnectionError(
            endpoint_url="https://privado.invalid"
        )
        with self.assertLogs("manejadores.errores", level="ERROR") as logs:
            mensaje = voz(self.solicitar("IniciarPartidaIntent"))
        self.assertIn("No pude completar", mensaje)
        self.assertNotIn("privado.invalid", str(logs.output))
        self.iot.get_thing_shadow.side_effect = self.leer_shadow
        self.iot.update_thing_shadow.side_effect = ClientError({"Error": {
            "Code": "UnauthorizedException", "Message": "privado",
        }}, "UpdateThingShadow")
        with self.assertLogs("manejadores.errores", level="ERROR"):
            mensaje = voz(self.solicitar("CambiarModoIntent", {
                "modo": slot_resuelto("modo", "manual"),
            }))
        self.assertNotIn("Envié", mensaje)
        self.assertIn("No pude completar", mensaje)

    def test_tapa_retirada_del_dispatcher_y_ayuda(self):
        for intent in ("AMAZON.HelpIntent", "AMAZON.FallbackIntent"):
            self.assertNotIn("tapa", voz(self.solicitar(intent)))
        with self.assertLogs("manejadores.errores", level="ERROR"):
            mensaje = voz(self.solicitar("ConsultarTapaIntent"))
        self.assertIn("No pude completar", mensaje)
        self.iot.get_thing_shadow.assert_not_called()

    def test_usuario_de_sesion_no_sustituye_context_system(self):
        self.solicitar("ConsultarModoIntent", usuario="usuario-02")
        self.repo.obtener.assert_called_once_with("usuario-02")
        self.iot.get_thing_shadow.assert_called_once_with(
            thingName="caja-enigmas-02"
        )

    def test_publicador_rechaza_temas_y_desafios_invalidos(self):
        for nombre, desafio in (("otra-caja", "memoria"),
                                ("caja-enigmas-01", "ninguno")):
            with self.assertRaises(ValueError):
                self.publicador.publicar_pista(nombre, desafio)
        self.iot.publish.assert_not_called()


if __name__ == "__main__":
    unittest.main()
