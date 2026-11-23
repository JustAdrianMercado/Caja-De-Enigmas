
import json


MODOS = ("juego", "manual")
DIFICULTADES = ("facil", "normal", "dificil")
RANGOS_PARAMETROS = {
    "distanciaObjetivoCm": (10, 40),
    "umbralLuzPorcentaje": (10, 95),
}
VALORES_REPORTADOS = {
    "modo": MODOS,
    "dificultad": DIFICULTADES,
    "longitudSecuencia": (3, 4, 6),
    "toleranciaCm": (6, 4, 2),
}
CAMPOS_PARAMETROS = (
    "dificultad", "longitudSecuencia", "toleranciaCm",
    *RANGOS_PARAMETROS,
)


class ServicioShadow:
    """Acceso al Shadow clásico mediante un cliente boto3 inyectado."""

    def __init__(self, cliente_iot):
        self.cliente_iot = cliente_iot

    def leer_estado(self, thing_name):
        """Lee desired y reported juntos, en una única versión del Shadow."""
        respuesta = self.cliente_iot.get_thing_shadow(
            thingName=thing_name
        )
        documento = self._leer_documento(respuesta)
        estado = documento.get("state", {})
        if not isinstance(estado, dict):
            raise ValueError("El estado del Shadow debe ser un objeto.")
        for seccion in ("reported", "desired"):
            if seccion in estado and not isinstance(estado[seccion], dict):
                raise ValueError("Las secciones del Shadow deben ser objetos.")
        return estado

    def leer_reported(self, thing_name):
        """Conserva la interfaz de vinculación y consultas existentes."""
        return self.leer_estado(thing_name).get("reported", {})

    def leer_modo(self, thing_name):
        """Devuelve el modo reportado o None si todavía no fue informado."""
        return self._leer_campo_reportado(thing_name, "modo")

    def leer_dificultad(self, thing_name):
        """Devuelve la dificultad reportada; nunca sustituye con desired."""
        return self._leer_campo_reportado(thing_name, "dificultad")

    def leer_parametros(self, thing_name):
        """Devuelve los parámetros presentes y válidos de un único reporte."""
        reportado = self.leer_reported(thing_name)
        parametros = {}
        for campo in CAMPOS_PARAMETROS:
            if campo in reportado:
                self._validar_campo(campo, reportado[campo])
                parametros[campo] = reportado[campo]
        return parametros

    def pedir_modo(self, thing_name, modo):
        """Pide un modo; al pasar a juego limpia la orden vieja de cerradura."""
        self._validar_campo("modo", modo)
        cambios = {"modo": modo}
        if modo == "juego":
            cambios["cerradura"] = None
        return self._actualizar_desired(thing_name, cambios)

    def pedir_dificultad(self, thing_name, dificultad):
        """No escribe longitud ni tolerancia: las determina el ESP32."""
        self._validar_campo("dificultad", dificultad)
        return self._actualizar_desired(thing_name, {"dificultad": dificultad})

    def pedir_parametro(self, thing_name, parametro, valor):
        """Acepta únicamente los dos parámetros modificables del contrato."""
        if not isinstance(parametro, str) or parametro not in RANGOS_PARAMETROS:
            raise ValueError("El parámetro no es modificable.")
        self._validar_campo(parametro, valor)
        return self._actualizar_desired(thing_name, {parametro: valor})

    def pedir_partida(self, thing_name):
        """Pide reported.solicitudPartida + 1 sin inventar estado faltante."""
        reportado = self.leer_reported(thing_name)
        if reportado.get("modo") != "juego":
            raise ValueError("Se necesita modo juego reportado para iniciar.")
        contador = reportado.get("solicitudPartida")
        if type(contador) is not int or contador < 0:
            raise ValueError("Falta una solicitudPartida reportada válida.")
        return self._actualizar_desired(
            thing_name, {"solicitudPartida": contador + 1}
        )

    def _leer_campo_reportado(self, thing_name, campo):
        reportado = self.leer_reported(thing_name)
        if campo not in reportado:
            return None
        self._validar_campo(campo, reportado[campo])
        return reportado[campo]

    @staticmethod
    def _validar_campo(campo, valor):
        if campo in RANGOS_PARAMETROS:
            minimo, maximo = RANGOS_PARAMETROS[campo]
            if type(valor) is int and minimo <= valor <= maximo:
                return
        elif campo in ("longitudSecuencia", "toleranciaCm"):
            if type(valor) is int and valor in VALORES_REPORTADOS[campo]:
                return
        elif isinstance(valor, str) and valor in VALORES_REPORTADOS[campo]:
            return
        raise ValueError(f"Valor inválido para {campo} según el contrato v2.")

    def pedir_cerradura(self, thing_name, estado):
        if estado not in ("abierta", "cerrada"):
            raise ValueError(
                "El estado debe ser 'abierta' o 'cerrada'"
            )

        return self._actualizar_desired(thing_name, {"cerradura": estado})

    def _actualizar_desired(self, thing_name, cambios):
        """Envía un parche sin copiar reported ni otras órdenes pendientes."""
        documento = {"state": {"desired": cambios}}
        respuesta = self.cliente_iot.update_thing_shadow(
            thingName=thing_name,
            payload=json.dumps(documento).encode("utf-8")
        )

        return self._leer_documento(respuesta)

    @staticmethod
    def _leer_documento(respuesta):
        payload = respuesta["payload"]
        try:
            documento = json.loads(payload.read())
        finally:
            payload.close()
        if not isinstance(documento, dict):
            raise ValueError("El documento Shadow debe ser un objeto.")
        return documento
