"""Pruebas de la regla "caja sin reportar", sin AWS: inventario, histórico y reloj falsos."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from vigilancia import (  # noqa: E402
    MILISEGUNDOS_POR_MINUTO, UltimoRegistro, VigilanciaCajas, fecha_iso,
)


AHORA_MS = 1_795_000_000_000  # 2026-11-18T11:06:40.000Z
LIMITE_MINUTOS = 15


def hace_minutos(minutos):
    return AHORA_MS - minutos * MILISEGUNDOS_POR_MINUTO


def lectura_de_hace(minutos):
    momento_ms = hace_minutos(minutos)
    return UltimoRegistro(tipo="lectura", fecha=fecha_iso(momento_ms), momento_ms=momento_ms)


def alerta_de_hace(minutos):
    momento_ms = hace_minutos(minutos)
    return UltimoRegistro(tipo="alerta", fecha=fecha_iso(momento_ms), momento_ms=momento_ms)


class InventarioFalso:
    def __init__(self, cajas):
        self.cajas = cajas

    def listar_cajas(self):
        return list(self.cajas)


class HistoricoFalso:
    """Último registro conocido de cada caja y alertas guardadas, en memoria."""

    def __init__(self, ultimos):
        self.ultimos = ultimos
        self.alertas = []

    def ultimo_registro(self, thing_name):
        return self.ultimos.get(thing_name)

    def guardar_alerta(self, alerta):
        self.alertas.append(alerta)


def crear_vigilancia(ultimos, minutos=LIMITE_MINUTOS):
    historico = HistoricoFalso(ultimos)
    vigilancia = VigilanciaCajas(
        inventario=InventarioFalso(sorted(ultimos)),
        historico=historico,
        minutos_sin_reporte=minutos,
        reloj_ms=lambda: AHORA_MS,
    )
    return vigilancia, historico


class TestVigilanciaCajas(unittest.TestCase):

    def test_caja_que_reporto_hace_poco_no_genera_alerta(self):
        vigilancia, historico = crear_vigilancia({"caja-enigmas-01": lectura_de_hace(1)})
        resumen = vigilancia.revisar()
        self.assertEqual(historico.alertas, [])
        self.assertEqual(resumen, {"cajasRevisadas": 1, "alertasNuevas": []})

    def test_justo_15_minutos_todavia_no_es_alerta(self):
        # El contrato dice "más de 15 min".
        vigilancia, historico = crear_vigilancia({"caja-enigmas-01": lectura_de_hace(15)})
        vigilancia.revisar()
        self.assertEqual(historico.alertas, [])

    def test_caja_sin_reportar_genera_la_alerta_del_contrato(self):
        ultima = lectura_de_hace(16)
        vigilancia, historico = crear_vigilancia({"caja-enigmas-01": ultima})
        resumen = vigilancia.revisar()
        self.assertEqual(resumen["alertasNuevas"], ["caja-enigmas-01"])
        self.assertEqual(historico.alertas, [{
            "thingName": "caja-enigmas-01",
            "ts": "2026-11-18T11:06:40.000Z#alerta",
            "tipo": "alerta",
            "categoriaAlerta": "sin_reporte",
            "tsGuardado": AHORA_MS,
            "ultimaLectura": ultima.fecha,
            "minutosSinReporte": 16,
        }])

    def test_caja_ya_avisada_no_repite_la_alerta(self):
        vigilancia, historico = crear_vigilancia({"caja-enigmas-01": alerta_de_hace(5)})
        vigilancia.revisar()
        self.assertEqual(historico.alertas, [])

    def test_caja_que_nunca_reporto_genera_alerta_sin_ultima_lectura(self):
        vigilancia, historico = crear_vigilancia({"caja-enigmas-02": None})
        vigilancia.revisar()
        self.assertEqual(len(historico.alertas), 1)
        alerta = historico.alertas[0]
        self.assertEqual(alerta["thingName"], "caja-enigmas-02")
        self.assertNotIn("ultimaLectura", alerta)
        self.assertNotIn("minutosSinReporte", alerta)

    def test_solo_avisa_de_las_cajas_silenciosas(self):
        vigilancia, historico = crear_vigilancia({
            "caja-enigmas-01": lectura_de_hace(2),
            "caja-enigmas-02": None,
            "caja-enigmas-03": lectura_de_hace(40),
        })
        resumen = vigilancia.revisar()
        self.assertEqual(resumen["cajasRevisadas"], 3)
        self.assertEqual(resumen["alertasNuevas"], ["caja-enigmas-02", "caja-enigmas-03"])

    def test_el_limite_se_puede_bajar_para_la_demo(self):
        vigilancia, historico = crear_vigilancia(
            {"caja-enigmas-01": lectura_de_hace(4)}, minutos=3
        )
        vigilancia.revisar()
        self.assertEqual(historico.alertas[0]["minutosSinReporte"], 4)

    def test_limite_invalido_se_rechaza(self):
        for invalido in (0, -5, "15", 2.5):
            with self.subTest(minutos=invalido):
                with self.assertRaises(ValueError):
                    crear_vigilancia({}, minutos=invalido)


class TestFechaIso(unittest.TestCase):

    def test_formato_igual_al_de_la_lambda_de_almacenamiento(self):
        self.assertEqual(fecha_iso(1_794_088_381_007), "2026-11-07T21:53:01.007Z")


if __name__ == "__main__":
    unittest.main()
