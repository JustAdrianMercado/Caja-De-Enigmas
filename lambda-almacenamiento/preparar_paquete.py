"""Genera y comprueba un ZIP de código puro para el runtime Python 3.12."""

import ast
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


CARPETA = Path(__file__).resolve().parent
ARCHIVOS = (
    "lambda_function.py", "validacion.py", "enriquecimiento.py",
    "servicios/__init__.py", "servicios/repositorio_historico.py",
)
DESTINO = CARPETA / "dist" / "caja-enigmas-almacenamiento.zip"


def preparar_paquete():
    """Empaqueta solo fuentes explícitas, sin tests, datos ni configuración."""
    DESTINO.parent.mkdir(exist_ok=True)
    for nombre in ARCHIVOS:
        ast.parse((CARPETA / nombre).read_text(encoding="utf-8"),
                  filename=nombre, feature_version=(3, 12))
    with ZipFile(DESTINO, "w", compression=ZIP_DEFLATED) as paquete:
        for nombre in ARCHIVOS:
            paquete.write(CARPETA / nombre, arcname=nombre)
    with ZipFile(DESTINO) as paquete:
        if paquete.testzip() is not None:
            raise RuntimeError("Falló la verificación de integridad del ZIP.")
        if set(paquete.namelist()) != set(ARCHIVOS):
            raise RuntimeError("El ZIP contiene archivos inesperados.")
    return DESTINO


if __name__ == "__main__":
    print(preparar_paquete())
