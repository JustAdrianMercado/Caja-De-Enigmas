"""Regla de la Nube: detecta las cajas que dejaron de reportar (contrato, sección 9.3).

Lógica pura: recibe el inventario de cajas, el histórico y el reloj ya armados, así se
prueba sin AWS (tests/test_vigilancia.py).
"""

from dataclasses import dataclass
from datetime import datetime, timedelta, timezone


TIPO_LECTURA = "lectura"
TIPO_ALERTA = "alerta"
CATEGORIA_SIN_REPORTE = "sin_reporte"
MILISEGUNDOS_POR_MINUTO = 60_000
EPOCH = datetime(1970, 1, 1, tzinfo=timezone.utc)


@dataclass(frozen=True)
class UltimoRegistro:
    """Lo más nuevo que el histórico sabe de una caja: su última lectura o una alerta."""

    tipo: str           # TIPO_LECTURA o TIPO_ALERTA.
    fecha: str          # ISO-8601 UTC con ms, sin el sufijo de la clave "ts".
    momento_ms: int     # Epoch en ms: tsRegla en las lecturas, tsGuardado en las alertas.


def fecha_iso(epoch_ms):
    """Mismo formato que la Lambda de almacenamiento: 2026-11-21T15:20:03.120Z."""
    fecha = EPOCH + timedelta(milliseconds=epoch_ms)
    return fecha.isoformat(timespec="milliseconds").replace("+00:00", "Z")


class VigilanciaCajas:
    """Guarda una alerta por cada caja sin lecturas en más de `minutos_sin_reporte`."""

    def __init__(self, inventario, historico, minutos_sin_reporte, reloj_ms):
        if type(minutos_sin_reporte) is not int or minutos_sin_reporte <= 0:
            raise ValueError("minutos_sin_reporte debe ser un entero positivo.")
        self.inventario = inventario
        self.historico = historico
        self.limite_ms = minutos_sin_reporte * MILISEGUNDOS_POR_MINUTO
        self.reloj_ms = reloj_ms

    def revisar(self):
        """Revisa todas las cajas y devuelve un resumen para los logs."""
        ahora_ms = self.reloj_ms()
        cajas = self.inventario.listar_cajas()
        alertas_nuevas = []
        for thing_name in cajas:
            alerta = self.revisar_caja(thing_name, ahora_ms)
            if alerta is not None:
                self.historico.guardar_alerta(alerta)
                alertas_nuevas.append(thing_name)
        return {"cajasRevisadas": len(cajas), "alertasNuevas": alertas_nuevas}

    def revisar_caja(self, thing_name, ahora_ms):
        """Devuelve el ítem de alerta a guardar, o None si la caja está bien o ya avisada."""
        ultimo = self.historico.ultimo_registro(thing_name)
        if ultimo is None:
            # Nunca reportó (p. ej., caja-enigmas-02, que no tiene placa).
            return self.armar_alerta(thing_name, ahora_ms, ultima_lectura=None)
        if ultimo.tipo == TIPO_ALERTA:
            return None  # Ya avisada: no se repite hasta que vuelva a reportar.
        if ahora_ms - ultimo.momento_ms <= self.limite_ms:
            return None
        return self.armar_alerta(thing_name, ahora_ms, ultima_lectura=ultimo)

    @staticmethod
    def armar_alerta(thing_name, ahora_ms, ultima_lectura):
        """Ítem de la tabla de histórico; categoriaAlerta lo pone en el GSI de alertas."""
        alerta = {
            "thingName": thing_name,
            "ts": f"{fecha_iso(ahora_ms)}#{TIPO_ALERTA}",
            "tipo": TIPO_ALERTA,
            "categoriaAlerta": CATEGORIA_SIN_REPORTE,
            "tsGuardado": ahora_ms,
        }
        if ultima_lectura is not None:
            alerta["ultimaLectura"] = ultima_lectura.fecha
            alerta["minutosSinReporte"] = (
                (ahora_ms - ultima_lectura.momento_ms) // MILISEGUNDOS_POR_MINUTO
            )
        return alerta
