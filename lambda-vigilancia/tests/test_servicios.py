"""Pruebas de los adaptadores de AWS con dobles de la tabla y del cliente de IoT."""

import sys
import unittest
from decimal import Decimal
from pathlib import Path
from unittest.mock import MagicMock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from servicios.inventario_things import InventarioThings  # noqa: E402
from servicios.repositorio_historico import (  # noqa: E402
    MAXIMO_PAGINAS, RepositorioHistorico,
)


def item_lectura(fecha, ts_regla):
    # Así lo guarda la Lambda de almacenamiento: números como Decimal al leerlos.
    return {"thingName": "caja-enigmas-01", "ts": f"{fecha}#lectura", "tipo": "lectura",
            "tsRegla": Decimal(ts_regla), "distanciaCm": Decimal(30)}


class TestRepositorioHistorico(unittest.TestCase):

    def setUp(self):
        self.tabla = MagicMock()
        self.repositorio = RepositorioHistorico(self.tabla)

    def test_pide_lo_mas_nuevo_primero_y_solo_lecturas_o_alertas(self):
        self.tabla.query.return_value = {"Items": []}
        self.repositorio.ultimo_registro("caja-enigmas-01")
        argumentos = self.tabla.query.call_args.kwargs
        self.assertFalse(argumentos["ScanIndexForward"])
        self.assertEqual(argumentos["ExpressionAttributeValues"][":thing"], "caja-enigmas-01")
        self.assertEqual(argumentos["FilterExpression"], "#tipo IN (:lectura, :alerta)")

    def test_convierte_la_ultima_lectura(self):
        self.tabla.query.return_value = {
            "Items": [item_lectura("2026-11-21T15:20:03.120Z", 1795792803120)]
        }
        ultimo = self.repositorio.ultimo_registro("caja-enigmas-01")
        self.assertEqual(ultimo.tipo, "lectura")
        self.assertEqual(ultimo.fecha, "2026-11-21T15:20:03.120Z")
        self.assertEqual(ultimo.momento_ms, 1795792803120)
        self.assertIsInstance(ultimo.momento_ms, int)

    def test_una_alerta_usa_su_hora_de_guardado(self):
        self.tabla.query.return_value = {"Items": [{
            "thingName": "caja-enigmas-01", "ts": "2026-11-21T15:40:00.000Z#alerta",
            "tipo": "alerta", "tsGuardado": Decimal(1795794000000),
        }]}
        ultimo = self.repositorio.ultimo_registro("caja-enigmas-01")
        self.assertEqual(ultimo.tipo, "alerta")
        self.assertEqual(ultimo.momento_ms, 1795794000000)

    def test_sigue_paginando_si_una_pagina_solo_tenia_eventos(self):
        # Con FilterExpression, una página puede volver vacía pero con más por leer.
        self.tabla.query.side_effect = [
            {"Items": [], "LastEvaluatedKey": {"ts": "a"}},
            {"Items": [item_lectura("2026-11-21T15:00:00.000Z", 1795791600000)]},
        ]
        ultimo = self.repositorio.ultimo_registro("caja-enigmas-01")
        self.assertEqual(ultimo.fecha, "2026-11-21T15:00:00.000Z")
        segunda = self.tabla.query.call_args_list[1].kwargs
        self.assertEqual(segunda["ExclusiveStartKey"], {"ts": "a"})

    def test_caja_sin_items_devuelve_none(self):
        self.tabla.query.return_value = {"Items": []}
        self.assertIsNone(self.repositorio.ultimo_registro("caja-enigmas-02"))

    def test_la_busqueda_tiene_un_tope_de_paginas(self):
        self.tabla.query.return_value = {"Items": [], "LastEvaluatedKey": {"ts": "x"}}
        self.assertIsNone(self.repositorio.ultimo_registro("caja-enigmas-01"))
        self.assertEqual(self.tabla.query.call_count, MAXIMO_PAGINAS)

    def test_guardar_alerta_hace_put_item(self):
        alerta = {"thingName": "caja-enigmas-01", "ts": "x#alerta"}
        self.repositorio.guardar_alerta(alerta)
        self.tabla.put_item.assert_called_once_with(Item=alerta)


class TestInventarioThings(unittest.TestCase):

    def test_recorre_todas_las_paginas_y_filtra_por_prefijo(self):
        cliente = MagicMock()
        cliente.list_things.side_effect = [
            {"things": [{"thingName": "caja-enigmas-02"}, {"thingName": "otro-objeto"}],
             "nextToken": "pagina-2"},
            {"things": [{"thingName": "caja-enigmas-01"}]},
        ]
        cajas = InventarioThings(cliente, "caja-enigmas-").listar_cajas()
        self.assertEqual(cajas, ["caja-enigmas-01", "caja-enigmas-02"])
        self.assertEqual(cliente.list_things.call_args_list[1].kwargs["nextToken"], "pagina-2")


if __name__ == "__main__":
    unittest.main()
