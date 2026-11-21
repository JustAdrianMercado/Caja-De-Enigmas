# Plan de Adrián Mercado — Firmware de integración, Shadow, AWS IoT y lógica de la Nube

**Rama:** `feat/firmware-aws-p4` · **Archivos a tu cargo:**
- `docs/contratos.md` y `firmware/platformio.ini`;
- `firmware/src/main.cpp`;
- en `firmware/include|src/`: `Desafio.h`, `ParametrosJuego.*`, `AlmacenParametros.*`, `EstadoShadow.*`, `PartidaEnigmas.*`, `ColaEventos.*`, `ConexionAwsIot.*`, `ConexionWifi.*`, `CodigoVinculacion.*` y `ConsolaSerial.*`;
- `firmware/test/**`;
- `lambda-vigilancia/**`;
- en `config/aws/`: `politica-iot.json`, `regla-vigilancia.json` y `politica-rol-vigilancia.json`.

Ver los acuerdos en [p4-00-resumen.md](p4-00-resumen.md). Al cerrar cada iteración, llenás tu [registro](#registro-de-lo-aplicado) al final de este archivo.

Sos el responsable de que el objeto se comporte como un juego, de que lo sincronice con el Shadow y de la única regla de la Nube. Santiago te entrega los desafíos, que implementan tu interfaz `Desafio.h`. Mateo guarda lo que publicás.

## Iteración 0 — vie 20: contrato v2 e interfaces (desbloquea a los demás)

1. ~~Contrato v2~~: **hecho**. Aprobado el 20/11 y publicado en [docs/contratos.md](../contratos.md).
2. Publicar en `main`:
   - **`Desafio.h`**: la interfaz común. Incluye `enum class ResultadoDesafio { EnCurso, Superado, Fallado }` y los métodos `iniciar(const ParametrosJuego&)`, `actualizar()`, `detener()`, `resultado()` y `nombre()`. Un comentario fija la regla de que ningún desafío bloquea `loop()`.
   - **`ParametrosJuego.h`**: los parámetros (`longitudSecuencia`, `toleranciaCm`, `distanciaObjetivoCm` y `umbralLuzPorcentaje`), la tabla de dificultades y la validación de rangos, como lógica pura, para probarla en la PC.
3. **Quitar la tapa**: borrar `EstadoTapa` y el uso del reed en `EstadoShadow` y `main.cpp`. `main` tiene que compilar y pasar `pio test -e pruebas`. `SensorTapa.*` lo borra Santiago junto con `firmware/pruebas/reed`, que es lo único que todavía lo usa.
4. **AWS (consola):**
   - ampliar `CajaEnigmasPolitica` con `iot:Publish` sobre `caja-enigmas/${iot:Connection.Thing.ThingName}/eventos`;
   - exportarla a `config/aws/politica-iot.json`;
   - pasarle a Mateo el tema y un payload de ejemplo.

**Listo cuando:** las interfaces y la política ampliada están en `main`, `main` compila sin el reed, y Santiago, Mateo y Lucía pueden empezar sin preguntarte nada.

## Iteración 1 — sáb 21: lógica del juego probada en la PC

- **`EstadoShadow` v2:** se agregan `modo`, `dificultad`, los parámetros, `solicitudPartida`, `estadoPartida`, `desafioActual`, `distanciaCm` y `luzPorcentaje`. Lee del `delta` cada atributo por separado y descarta los inválidos. Ampliar `firmware/test/test_estado_shadow`.
- **`PartidaEnigmas`:** la máquina de estados del Edge, sin Arduino:
  - recorre `esperando → memoria → distancia → luz → resuelta` con tres `Desafio&` que recibe en el constructor;
  - inicia con el pulsador de inicio o con un `solicitudPartida` nuevo;
  - cuenta los intentos y la duración de cada desafío, y produce los eventos del contrato;
  - en modo `manual` queda en pausa;
  - el tiempo se recibe desde afuera, así que no usa `millis()` y se puede probar.
- **Tests** en `firmware/test/test_partida_enigmas/`, con un `DesafioFalso` que se supera o falla cuando el test lo indica. Agregar `PartidaEnigmas.cpp` y `ParametrosJuego.cpp` a `build_src_filter` del entorno `pruebas`.
- **`AlmacenParametros`:** guarda el modo, la dificultad y los parámetros en NVS con `Preferences`, igual que `CodigoVinculacion`. Así el juego queda configurado después de un reinicio sin red.

**Listo cuando:** `pio test -e pruebas` pasa los casos de partida completa, fallo y reintento en la memoria, modo manual, solicitud repetida (no inicia dos veces) y parámetros fuera de rango.

## Iteración 2 — dom 22: eventos, cola sin conexión e integración

**Mañana**, con la placa suelta prestada por Santiago o, si no la hay, con los tests y el cliente MQTT:
- **`ConexionAwsIot`:** agregar `publicarEvento(json)` en `caja-enigmas/<thing>/eventos`.
- **`ColaEventos`:**
  - un buffer circular de tamaño fijo (por ejemplo, 20 eventos) para los que se producen sin conexión, con `tsObjeto` si ya hay hora NTP;
  - al reconectar se vacía en orden;
  - si se llena, se descarta el más viejo y se cuenta.
  - Este comportamiento se declara en el RNF de operación sin conexión.
- **Lecturas:** `distanciaCm` y `luzPorcentaje` van en el reporte periódico de 60 s y en cada cambio de desafío.

**Tarde, con Santiago y la caja:** integrar sus desafíos en `main.cpp`. Revisar juntos el PR y jugar una partida completa. Comprobar en la consola que el `reported` cambia y que llegan los eventos al tema.

**Noche: Lambda `caja-enigmas-vigilancia`** (Python 3.12, en `lambda-vigilancia/`, con tests):
- EventBridge `rate(5 minutes)`. Busca el último reporte de cada caja (patrón de acceso 5).
- Si pasaron más de `MINUTOS_SIN_REPORTE=15`, guarda una alerta en `CajaEnigmasHistorico`, sin repetirla si ya existe una abierta para esa caja.
- Variables de entorno: `TABLA_HISTORICO`, `PREFIJO_THING` y `MINUTOS_SIN_REPORTE`.
- Rol con mínimo privilegio, exportado a `config/aws/politica-rol-vigilancia.json`.
- La regla de EventBridge, exportada a `config/aws/regla-vigilancia.json`.
- Mientras Mateo no tenga datos reales, probar con ítems de prueba puestos a mano en la tabla.

## Iteración 3 — lun 23: pruebas de RNF del firmware y de la Nube

| Prueba | Procedimiento |
|---|---|
| Operación sin conexión | Cortar el WiFi en medio de una partida, 3 veces. El juego sigue y el servo abre. Al reconectar, los eventos de la cola llegan a DynamoDB: contar los enviados y los guardados. |
| Reconexión con `desired` pendiente | Cambiar el modo y la dificultad por voz con la placa sin red. Al reconectar se aplican solos. |
| Tiempo de respuesta de la Nube | Apagar la placa y medir cuánto tarda en aparecer la alerta (esperado: ≤ 15 + 5 min). 3 veces. |
| Tiempo de respuesta del Edge | Con Santiago (ver su plan). |
| Mensajes por día | Contar las publicaciones durante 10 min (cliente MQTT) y pasarle a Lucía los mensajes por día para la estimación de costos. |

## Iteración 4 — mar 24: informe y entrega

- En `INFORME_P4.md`:
  - **diagrama de arquitectura**, con las reglas, las Lambdas y las tablas;
  - **especificación del Shadow v2**;
  - **diagrama de clases del firmware**, con Santiago;
  - **diagrama de estados de la lógica de control**, con la justificación Edge/Nube;
  - secuencias de cada regla automática;
  - configuración de IoT y de la vigilancia, con los permisos justificados.
- Revisar con Lucía que el repo no tenga secretos y que el enlace sea accesible.

## Para la defensa: qué repasar de los demás

- Los desafíos de Santiago: antirrebote, por qué el HC-SR04 necesita el divisor y por qué la LDR va en ADC1.
- La Lambda de almacenamiento de Mateo: validaciones, enriquecimiento, claves, GSI y TTL de la tabla, y las sentencias SQL de las reglas.
- Los handlers nuevos de la skill: cómo se rechaza "abre la caja" en modo juego y cómo se valida un parámetro.
- Los mockups de Lucía: de qué consulta sale cada dato.

## Registro de lo aplicado

Llenar al cerrar cada iteración, con lo que **de verdad** quedó hecho. Si un campo no aplica, "—". Qué va en cada campo y a qué sección del informe alimenta: [p4-00-resumen.md](p4-00-resumen.md#registro-de-lo-aplicado-en-cada-iteración).

### Iteración 0 — vie 20
- **Qué quedó funcionando:**
  - Contrato v2 aprobado en `docs/contratos.md` (PR #5).
  - `Desafio.h`: la interfaz de los desafíos, con `detener()`.
  - `ParametrosJuego`: la tabla de dificultades, los rangos de los parámetros y los textos de la dificultad, con 12 tests nativos.
  - `EstadoShadow` y `main.cpp` sin la tapa: el `reported` ya no incluye `tapa`.
- **Archivos, commits y PR:**
  - nuevos: `firmware/include/Desafio.h`, `firmware/include/ParametrosJuego.h`, `firmware/src/ParametrosJuego.cpp` y `firmware/test/test_parametros_juego/`;
  - cambiados: `EstadoShadow.*`, `main.cpp`, `test_estado_shadow`, `firmware/platformio.ini` (`ParametrosJuego.cpp` en el entorno `pruebas`), `config/aws/politica-iot.json` y `docs/contratos.md` (sección 10);
  - rama `feat/firmware-aws-p4`; PR: pendiente.
- **Decisiones y por qué:**
  - **`detener()` en la interfaz:** sin él, al pasar a modo manual o reiniciar una partida, un LED del desafío en curso podía quedar encendido.
  - **`Superado` y `Fallado` son finales** hasta el próximo `iniciar()`: así `PartidaEnigmas` lee el resultado sin que cambie entre dos vueltas de `loop()`.
  - **La validación recibe `int32_t`:** un 276 que llegue del Shadow se rechaza, en lugar de convertirse en 20 al guardarlo en un `uint8_t`.
  - **`SensorTapa.*` lo borra Santiago** con `pruebas/reed`: ese diagnóstico es lo único que todavía usa la clase, y él reutiliza el antirrebote en `Pulsador`. Así los dos proyectos de PlatformIO siguen compilando en `main`.
- **Configuración en AWS (sin secretos):** sentencia nueva `PublicarSusEventos` en `config/aws/politica-iot.json`. **Pendiente:** aplicarla en la consola como versión nueva de `CajaEnigmasPolitica`.
- **Pruebas y valores medidos:**
  - `pio test -e pruebas`: 30 de 30 (18 de `EstadoShadow` y 12 de `ParametrosJuego`);
  - `pio run -e caja`: compila;
  - `Desafio.h` compila con una implementación de prueba, sin advertencias con `-Wall -Wextra`.
- **Evidencias:** —
- **Cambios al contrato o al plan:**
  - Sección 10: `detener()`, resultados finales y copia de los parámetros en `iniciar()`.
  - Plan: el borrado de `SensorTapa.*` pasa a Santiago.
  - El tema y el payload de ejemplo para Mateo ya están en la sección 3.2 del contrato.
- **Problemas y pendientes:**
  - Aplicar la política en la consola de IoT.
  - El Shadow de `caja-enigmas-01` conserva el `reported.tapa` viejo, porque el ESP32 ya no lo reporta pero tampoco lo borra. Se borra publicando una vez `{"state":{"reported":{"tapa":null}}}`.
  - La sesión de AWS CLI estaba vencida, así que no se comparó la política del repo con la activa.
  - Para las pruebas nativas, `C:\MinGW\bin` tiene que estar en el `PATH` (g++).

### Iteración 1 — sáb 21
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 2 — dom 22
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 3 — lun 23
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**
