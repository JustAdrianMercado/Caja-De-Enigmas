"""Genera el ZIP de caja-enigmas-vigilancia para subirlo desde la consola de Lambda.

No incluye dependencias: boto3 ya viene en el runtime Python 3.12 de Lambda.
Uso, desde esta carpeta: python preparar_paquete.py
"""

import ast
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


CARPETA = Path(__file__).resolve().parent
ARCHIVOS = (
    "lambda_function.py", "vigilancia.py",
    "servicios/__init__.py", "servicios/inventario_things.py",
    "servicios/repositorio_historico.py",
)
DESTINO = CARPETA / "dist" / "caja-enigmas-vigilancia.zip"


def preparar_paquete():
    """Empaqueta solo el código de producción: sin tests ni configuración."""
    DESTINO.parent.mkdir(exist_ok=True)
    for nombre in ARCHIVOS:
        # Falla antes de empaquetar si algo no es sintaxis válida de Python 3.12.
        ast.parse((CARPETA / nombre).read_text(encoding="utf-8"),
                  filename=nombre, feature_version=(3, 12))
    with ZipFile(DESTINO, "w", compression=ZIP_DEFLATED) as paquete:
        for nombre in ARCHIVOS:
            paquete.write(CARPETA / nombre, arcname=nombre)
    with ZipFile(DESTINO) as paquete:
        if paquete.testzip() is not None:
            raise RuntimeError("El ZIP generado está dañado.")
        if set(paquete.namelist()) != set(ARCHIVOS):
            raise RuntimeError("El ZIP contiene archivos inesperados.")
    return DESTINO


if __name__ == "__main__":
    print(preparar_paquete())
