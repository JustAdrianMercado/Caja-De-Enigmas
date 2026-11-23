"""Contrato v2 y cadena completa con tabla, repositorio y reloj falsos."""

from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch


CARPETA = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CARPETA))
ESPECIFICACION = importlib.util.spec_from_file_location(
    "almacenamiento_local", CARPETA / "lambda_function.py"
)
MODULO = importlib.util.module_from_spec(ESPECIFICACION)
ESPECIFICACION.loader.exec_module(MODULO)
from enriquecimiento import construir_item
from servicios.repositorio_historico import RepositorioHistorico
from validacion import CAMPOS_POR_EVENTO, MensajeInvalido, validar_mensaje


HORA_GUARDADO_MS = 1795274405120
EJEMPLOS = json.loads(
    (CARPETA / "ejemplos/mensajes.json").read_text(encoding="utf-8")
)


class TablaFalsa:
    """Simula reemplazos por PK/SK sin conectarse a DynamoDB."""

    def __init__(self):
        self.items = {}
        self.escrituras = []

    def put_item(self, Item):
        self.escrituras.append(deepcopy(Item))
        self.items[(Item["thingName"], Item["ts"])] = deepcopy(Item)
        return {}


class TestAlmacenamiento(unittest.TestCase):
    """Comprueba las dos reglas y los descartes con dependencias locales."""

    def setUp(self):
        self.tabla = TablaFalsa()
        self.repositorio = RepositorioHistorico(self.tabla)
        self.reloj = Mock(return_value=HORA_GUARDADO_MS)

    def ejemplo(self, nombre):
        return deepcopy(next(e["entradaLambda"] for e in EJEMPLOS
                             if e["nombre"] == nombre))

    def procesar(self, mensaje):
        with self.assertLogs(MODULO.REGISTRADOR, level="INFO") as registros:
            respuesta = MODULO.procesar_mensaje(
                mensaje, self.repositorio, self.reloj
            )
        return respuesta, registros

    def comprobar_descarte(self, mensaje):
        anteriores = len(self.tabla.escrituras)
        respuesta, registros = self.procesar(mensaje)
        self.assertTrue(respuesta["descartado"])
        self.assertFalse(respuesta["persistido"])
        self.assertEqual(len(self.tabla.escrituras), anteriores)
        self.assertEqual(registros.records[0].levelname, "WARNING")

    def test_todos_los_ejemplos_y_seis_eventos(self):
        for ejemplo in EJEMPLOS:
            with self.subTest(nombre=ejemplo["nombre"]):
                mensaje = deepcopy(ejemplo["entradaLambda"])
                original = deepcopy(mensaje)
                respuesta, registros = self.procesar(mensaje)
                self.assertEqual(respuesta, {
                    "recibido": True, "persistido": True, "descartado": False,
                })
                item = self.tabla.escrituras[-1]
                self.assertEqual(mensaje, original)
                self.assertEqual(item["tsGuardado"], HORA_GUARDADO_MS)
                self.assertEqual(item["tsRegla"], mensaje["tsRegla"])
                self.assertIsInstance(item["ts"], str)
                self.assertNotIn("tipoMensaje", item)
                self.assertNotIn("categoriaAlerta", item)
                if mensaje.get("tipoMensaje") == "evento":
                    self.assertEqual(item["tipo"], "evento")
                    self.assertEqual(item["evento"], mensaje["tipo"])
                    self.assertNotIn("expiraEn", item)
                else:
                    self.assertEqual(item["tipo"], "lectura")
                    self.assertNotIn("evento", item)
                self.assertTrue(json.loads(
                    registros.records[-1].getMessage())["persistido"])

    def test_pista_y_cerradura_no_exigen_id_partida(self):
        for nombre in ("pista", "cerradura"):
            self.procesar(self.ejemplo(nombre))
            self.assertNotIn("idPartida", self.tabla.escrituras[-1])

    def test_cada_campo_obligatorio_de_cada_evento(self):
        for tipo, campos in CAMPOS_POR_EVENTO.items():
            for campo in campos:
                with self.subTest(tipo=tipo, campo=campo):
                    mensaje = self.ejemplo(tipo)
                    del mensaje[campo]
                    self.comprobar_descarte(mensaje)
        self.reloj.assert_not_called()

    def test_distancia_sola_luz_sola_y_ambas(self):
        for sensores in ({"distanciaCm": 2}, {"luzPorcentaje": 0},
                         {"distanciaCm": 400, "luzPorcentaje": 100}):
            mensaje = {"tipo": "lectura", "thingName": "caja-enigmas-02",
                       "tsRegla": 0, **sensores}
            self.procesar(mensaje)
            for campo, valor in sensores.items():
                self.assertEqual(self.tabla.escrituras[-1][campo], valor)

    def test_lectura_sin_sensores(self):
        mensaje = self.ejemplo("lectura_solo_luz")
        del mensaje["luzPorcentaje"]
        self.comprobar_descarte(mensaje)

    def test_rangos_y_tipos_invalidos(self):
        casos = {
            "distanciaCm": (
                "lectura_distancia_y_luz", [1, 401, True, 20.0, "20", None]
            ),
            "luzPorcentaje": ("lectura_solo_luz", [-1, 101, False, [], {}]),
            "idPartida": ("desafio_superado", [0, -1, True, 1.5, "12"]),
            "duracionMs": ("desafio_superado", [-1, 3600001, True, "1"]),
            "intentos": ("desafio_superado", [0, 100, True, None]),
        }
        for campo, (nombre, valores) in casos.items():
            for valor in valores:
                with self.subTest(campo=campo, valor=valor):
                    mensaje = self.ejemplo(nombre)
                    mensaje[campo] = valor
                    self.comprobar_descarte(mensaje)

    def test_limites_validos_de_evento(self):
        for duracion in (0, 3600000):
            for intentos in (1, 99):
                mensaje = self.ejemplo("desafio_superado")
                mensaje.update(
                    idPartida=1, duracionMs=duracion, intentos=intentos
                )
                self.procesar(mensaje)

    def test_enums_invalidos(self):
        for nombre, campo in (("pista", "desafio"), ("pista", "origen"),
                              ("cerradura", "cerradura"),
                              ("partida_iniciada", "dificultad")):
            for valor in ("desconocido", [], {}, None):
                with self.subTest(nombre=nombre, campo=campo, valor=valor):
                    mensaje = self.ejemplo(nombre)
                    mensaje[campo] = valor
                    self.comprobar_descarte(mensaje)

    def test_origenes_especificos_y_dificultades(self):
        for tipo, origen in (("pista", "edge"), ("pista", "nube"),
                             ("partida_iniciada", "nube")):
            mensaje = self.ejemplo(tipo)
            mensaje["origen"] = origen
            self.comprobar_descarte(mensaje)
        for dificultad in ("facil", "normal", "dificil"):
            for origen in ("voz", "edge"):
                mensaje = self.ejemplo("partida_iniciada")
                mensaje.update(dificultad=dificultad, origen=origen)
                self.procesar(mensaje)
        for origen in ("voz", "edge", "nube"):
            mensaje = self.ejemplo("cerradura")
            mensaje["origen"] = origen
            self.procesar(mensaje)

    def test_distingue_las_dos_entradas_de_reglas(self):
        for tipo in (None, "evento", "alerta", "lectura", [], {}):
            mensaje = self.ejemplo("pista")
            mensaje["tipo"] = tipo
            self.comprobar_descarte(mensaje)
        mensaje = self.ejemplo("pista")
        del mensaje["tipoMensaje"]
        self.comprobar_descarte(mensaje)
        for valor in (None, "lectura", "evento"):
            mensaje = self.ejemplo("lectura_solo_luz")
            mensaje["tipoMensaje"] = valor
            self.comprobar_descarte(mensaje)

    def test_metadatos_obligatorios(self):
        for campo in ("thingName", "tsRegla"):
            mensaje = self.ejemplo("pista")
            del mensaje[campo]
            self.comprobar_descarte(mensaje)
        nombres = (None, "caja-enigmas-1", "otra-01", "caja-enigmas-01\n", {})
        for nombre in nombres:
            mensaje = self.ejemplo("pista")
            mensaje["thingName"] = nombre
            self.comprobar_descarte(mensaje)
        for fecha in (None, -1, True, 1.2, "123", 253402300800000):
            mensaje = self.ejemplo("pista")
            mensaje["tsRegla"] = fecha
            self.comprobar_descarte(mensaje)

    def test_entradas_no_objeto_y_objeto_vacio(self):
        for mensaje in (None, [], "DATO_PRIVADO", 1, {}):
            self.comprobar_descarte(mensaje)

    def test_ts_objeto_tiene_prioridad(self):
        mensaje = self.ejemplo("desafio_superado")
        self.procesar(mensaje)
        item = self.tabla.escrituras[-1]
        self.assertEqual(item["ts"],
                         "2026-11-21T15:20:03.120Z#desafio_superado")
        self.assertEqual(item["tsObjeto"], mensaje["tsObjeto"])

    def test_sin_ts_objeto_usa_regla_sin_inventar_hora_objeto(self):
        mensaje = self.ejemplo("lectura_solo_luz")
        mensaje["tsRegla"] = 1123
        self.procesar(mensaje)
        item = self.tabla.escrituras[-1]
        self.assertEqual(item["ts"], "1970-01-01T00:00:01.123Z#lectura")
        self.assertNotIn("tsObjeto", item)
        self.assertFalse(any("latencia" in campo.lower() for campo in item))

    def test_timestamps_invalidos(self):
        for fecha in (None, "2026-02-30T00:00:00.000Z",
                      "2026-11-21T15:20:03Z", "2026-11-21T15:20:03.12Z",
                      "2026-11-21T15:20:03.120+00:00", "fecha", 123):
            mensaje = self.ejemplo("pista")
            mensaje["tsObjeto"] = fecha
            self.comprobar_descarte(mensaje)

    def test_ttl_solo_lecturas_desde_guardado_en_segundos(self):
        self.procesar(self.ejemplo("lectura_solo_luz"))
        self.assertEqual(self.tabla.escrituras[-1]["expiraEn"],
                         HORA_GUARDADO_MS // 1000 + 30 * 86400)
        self.procesar(self.ejemplo("pista"))
        self.assertNotIn("expiraEn", self.tabla.escrituras[-1])

    def test_configuracion_retencion(self):
        mensaje = validar_mensaje(self.ejemplo("lectura_solo_luz"))
        self.assertEqual(construir_item(mensaje, 1999, 1)["expiraEn"], 86401)
        for dias in (0, -1, True, "30"):
            with self.assertRaises(ValueError):
                construir_item(mensaje, HORA_GUARDADO_MS, dias)

    def test_campos_ajenos_no_se_guardan_ni_se_registran(self):
        mensaje = self.ejemplo("pista")
        mensaje.update(codigoVinculacion="DATO_PRIVADO", ts="DATO_PRIVADO",
                       expiraEn=1, categoriaAlerta="sin_reporte",
                       tsGuardado=1, evento="DATO_PRIVADO")
        _, registros = self.procesar(mensaje)
        self.assertNotIn("DATO_PRIVADO", str(registros.output))
        item = self.tabla.escrituras[-1]
        self.assertNotIn("DATO_PRIVADO", str(item))
        self.assertNotIn("expiraEn", item)
        self.assertNotIn("categoriaAlerta", item)
        self.assertEqual(item["tsGuardado"], HORA_GUARDADO_MS)
        self.assertEqual(item["evento"], "pista")

    def test_descarte_no_revela_valor_privado(self):
        mensaje = self.ejemplo("pista")
        mensaje["desafio"] = "DATO_PRIVADO\nINYECCION"
        _, registros = self.procesar(mensaje)
        self.assertNotIn("DATO_PRIVADO", str(registros.output))

    def test_colision_mismo_evento_misma_caja_y_milisegundo(self):
        mensaje = self.ejemplo("desafio_superado")
        self.procesar(mensaje)
        mensaje["intentos"] = 3
        self.procesar(mensaje)
        self.assertEqual(len(self.tabla.escrituras), 2)
        self.assertEqual(len(self.tabla.items), 1)
        guardado = next(iter(self.tabla.items.values()))
        self.assertEqual(guardado["intentos"], 3)

    def test_eventos_distintos_mismo_milisegundo_no_colisionan(self):
        self.procesar(self.ejemplo("pista"))
        self.procesar(self.ejemplo("cerradura"))
        self.assertEqual(len(self.tabla.items), 2)

    def test_lecturas_repetidas_y_colision_de_fecha(self):
        mensaje = self.ejemplo("lectura_solo_luz")
        self.procesar(mensaje)
        self.procesar(mensaje)
        self.assertEqual(len(self.tabla.escrituras), 2)
        self.assertEqual(len(self.tabla.items), 1)
        mensaje["tsRegla"] += 1
        self.procesar(mensaje)
        self.assertEqual(len(self.tabla.items), 2)

    def test_error_repositorio_no_confirma_ni_expone_detalles(self):
        repositorio = Mock()
        repositorio.guardar.side_effect = OSError("DATO_PRIVADO")
        with self.assertLogs(MODULO.REGISTRADOR, level="INFO") as registros:
            with self.assertRaisesRegex(RuntimeError,
                                        "Fallo de almacenamiento"):
                MODULO.procesar_mensaje(self.ejemplo("pista"), repositorio,
                                       self.reloj)
        self.assertNotIn("DATO_PRIVADO", str(registros.output))
        self.assertNotIn('"persistido": true', str(registros.output))

    def test_repositorio_propaga_error_del_cliente(self):
        tabla = Mock()
        tabla.put_item.side_effect = OSError("fallo simulado")
        with self.assertRaises(OSError):
            RepositorioHistorico(tabla).guardar({"thingName": "prueba"})

    def test_handler_con_fabrica_y_reloj_falsos(self):
        with patch.object(MODULO, "obtener_repositorio",
                          return_value=self.repositorio), patch.object(
                              MODULO, "hora_actual_ms", self.reloj
                          ), patch.dict(MODULO.os.environ,
                                        {"DIAS_RETENCION_LECTURAS": "30"}):
            respuesta = MODULO.lambda_handler(self.ejemplo("pista"), None)
        self.assertTrue(respuesta["persistido"])
        self.assertEqual(self.tabla.escrituras[-1]["tsGuardado"],
                         HORA_GUARDADO_MS)

    def test_configuracion_invalida_no_inicializa_aws(self):
        for valor in ("0", "-1", "DATO_PRIVADO"):
            with patch.dict(MODULO.os.environ,
                            {"DIAS_RETENCION_LECTURAS": valor}), patch.object(
                                MODULO, "obtener_repositorio") as fabrica:
                with self.assertRaisesRegex(RuntimeError,
                                            "Configuración de retención"):
                    MODULO.lambda_handler(self.ejemplo("pista"), None)
                fabrica.assert_not_called()

    def test_invalido_no_inicializa_sdk(self):
        with patch.object(MODULO, "obtener_repositorio") as fabrica:
            self.comprobar_descarte({})
        fabrica.assert_not_called()


if __name__ == "__main__":
    unittest.main()
