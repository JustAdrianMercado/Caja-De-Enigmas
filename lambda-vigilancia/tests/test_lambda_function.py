"""Pruebas de la configuración por variables de entorno (sin boto3 ni AWS)."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from lambda_function import leer_configuracion  # noqa: E402


class TestLeerConfiguracion(unittest.TestCase):

    def test_valores_por_defecto_del_contrato(self):
        self.assertEqual(leer_configuracion({}), {
            "tabla": "CajaEnigmasHistorico", "prefijo": "caja-enigmas-", "minutos": 15,
        })

    def test_la_demo_puede_bajar_el_limite(self):
        configuracion = leer_configuracion({"MINUTOS_SIN_REPORTE": "3"})
        self.assertEqual(configuracion["minutos"], 3)

    def test_limite_que_no_es_un_entero_positivo_falla(self):
        for invalido in ("quince", "0", "-1", "2.5"):
            with self.subTest(valor=invalido):
                with self.assertRaises(RuntimeError):
                    leer_configuracion({"MINUTOS_SIN_REPORTE": invalido})


if __name__ == "__main__":
    unittest.main()
