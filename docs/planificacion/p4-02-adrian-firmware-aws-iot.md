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
  - al pasar a modo `manual` abandona la partida (contrato, sección 9.1);
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
  - rama `feat/firmware-aws-p4`, PR #6.
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
  - **`PartidaEnigmas`**: la máquina de estados del Edge. Produce los eventos del contrato y decide cuándo se mueve la cerradura en el juego. Tiene 20 tests con desafíos falsos.
  - **`EstadoShadow` v2**: lee cada atributo del `delta` por separado, descarta los inválidos y reporta el estado completo del contrato v2. Tiene 32 tests.
  - **`ConfiguracionJuego`** (en `ParametrosJuego`): modo, dificultad y calibración. Incluye la regla de la cerradura en modo juego y la carga validada desde la NVS.
  - **`AlmacenParametros`**: guarda en la NVS la configuración, `idPartida` y `solicitudPartida`.
  - **`main.cpp`** (firmware 0.3.0):
    - aplica el modo, la dificultad y los parámetros del `delta`, los guarda en la NVS y reporta el Shadow v2;
    - mueve la cerradura por voz solo en modo manual;
    - **no se probó en la placa.**
- **Archivos, commits y PR:**
  - nuevos: `TiposCaja.h`, `EventoCaja.h`, `PartidaEnigmas.*`, `AlmacenParametros.*` y `firmware/test/test_partida_enigmas/`;
  - cambiados: `ParametrosJuego.*`, `EstadoShadow.*`, `main.cpp`, `ConexionAwsIot.h`, `firmware/platformio.ini`, los tests de `EstadoShadow` y `ParametrosJuego`, y `docs/contratos.md` (secciones 3.2 y 9.1);
  - rama `feat/firmware-aws-p4-it1`, PR #7.
- **Decisiones y por qué:**
  - **El juego avisa sus movimientos de la cerradura como eventos** (`cerradura` con origen `edge`), y `main.cpp` mueve el servo al recibirlos. Así la secuencia completa del contrato (cerrar, iniciar, superar ×3, abrir, resolver) se prueba en la PC.
  - **`TiposCaja.h`** junta los `enum` que comparten el Shadow, los eventos y la partida, para que esos módulos no dependan unos de otros.
  - **`PartidaEnigmas` maneja `solicitudPartida`:** la adopta siempre, también en modo manual, para que el `delta` se limpie. Solo inicia una partida en modo juego, y un valor repetido no hace nada.
  - **El `delta` se valida por tipo:** se rechazan `"20"` (texto), `70.5`, `-1` y `276`. Este último se habría convertido en 20 al guardarse en 8 bits.
  - **La NVS guarda los `enum` como números y los valida al cargar.** Un valor inválido vuelve al de fábrica sin cambiar los demás.
  - **Buffers:**
    - el de mensajes MQTT sube a 3 KB, porque el `get/accepted` v2 mide 1651 bytes en el peor caso y con 2 KB quedaba poco margen;
    - el del reporte sube a 512 bytes, y el reporte más largo ocupa 370.
  - **La duración de un desafío cuenta desde su primer intento**, y la duración y los intentos se saturan en el rango del contrato para que la Lambda no descarte el evento.
- **Configuración en AWS (sin secretos):** —
- **Pruebas y valores medidos:**
  - `pio test -e pruebas`: 70 de 70 (32 de `EstadoShadow`, 18 de `ParametrosJuego` y 20 de `PartidaEnigmas`). Cubren:
    - la partida completa;
    - el fallo y el reintento en la memoria;
    - el modo manual;
    - la solicitud repetida;
    - los parámetros fuera de rango;
    - el desborde de `millis()`.
  - `pio run -e caja`: compila, con 17,4 % de RAM (57 048 B) y 71,7 % de Flash.
  - Sin advertencias con `-Wall -Wextra` en la lógica pura.
- **Evidencias:** —
- **Cambios al contrato o al plan:**
  - Contrato 3.2: `duracionMs` cuenta desde el primer intento, y la duración y los intentos se saturan.
  - Contrato 9.1: pasar a manual no publica ningún evento, y una partida resuelta también vuelve a `esperando`.
  - Plan: "en modo manual queda en pausa" pasa a "abandona la partida", como dice el contrato.
- **Problemas y pendientes:**
  - `main.cpp` todavía no crea la `PartidaEnigmas`, porque faltan los desafíos de Santiago (iteración 2). Hasta entonces, `solicitudPartida` queda pendiente en el `delta` y `estadoPartida` se reporta siempre `esperando`.
  - Falta probarlo en la placa con Santiago: la NVS, el `delta` v2 y el tamaño real del `get/accepted`.
  - Con el firmware 0.3.0, la caja arranca en modo juego. Mientras no esté integrada la partida, en ese modo la cerradura no se abre por voz: hay que pasar a manual o usar el comando Serial `abrir`.

### Iteración 2 — dom 22 (terminada el lun 23)
- **Qué quedó funcionando** (compila y pasa los tests; **todavía no se probó en la placa ni en AWS**):
  - **Eventos:**
    - `EventoJson` arma el JSON del contrato, solo con los campos de cada tipo;
    - `RelojUtc` da el `tsObjeto`;
    - `ColaEventos` guarda 20 eventos, descarta el más viejo si se llena y descarta un evento rechazado 3 veces;
    - `ConexionAwsIot::publicarEvento` publica en el tema de eventos.
  - **Firmware 0.4.0 en `main.cpp`:**
    - partida integrada con los desafíos de Santiago, el pulsador de inicio y el inicio por voz;
    - `idPartida` y `solicitudPartida` guardados en la NVS;
    - lecturas de distancia y luz en el reporte;
    - todos los eventos pasan por la cola;
    - comando Serial `iniciar`.
  - **Lambda `caja-enigmas-vigilancia`**, con tests, el script de empaquetado y un README con los pasos de consola. La política del rol y la regla de EventBridge están en `config/aws/`.
  - **Borrador de mis secciones del informe**, integrado en el [informe de la P4](../../INFORME_P4.md).
- **Archivos, commits y PR:**
  - nuevos: `EventoJson.*`, `RelojUtc.*`, `ColaEventos.*`, `firmware/test/test_evento_json/`, `firmware/test/test_cola_eventos/`, `lambda-vigilancia/**`, `config/aws/politica-rol-vigilancia.json` y `config/aws/regla-vigilancia.json`;
  - cambiados: `main.cpp`, `ConexionAwsIot.*`, `firmware/platformio.ini` y `docs/contratos.md` (secciones 3.3, 6 y 9.3);
  - rama `feat/firmware-aws-p4-it2`, PR #10.
- **Decisiones y por qué:**
  - **Todos los eventos pasan por la cola, haya red o no**, así se publican siempre en orden. Sale uno por vuelta de `loop()`, porque cada publicación QoS 1 espera el PUBACK.
  - **El JSON se arma en el momento del evento**, así `tsObjeto` es la hora real y no la de la reconexión.
  - **Cada tipo lleva solo sus campos**, porque la Lambda de Mateo valida todos los campos que llegan.
  - **Un evento rechazado 3 veces se descarta.** Sin el permiso de la política, AWS corta la conexión en cada intento, y sin este límite la caja se reconectaría sin fin.
  - **`main.cpp` no mide la distancia mientras corre ese desafío**, porque `SensorDistancia::actualizar()` entrega cada medición una sola vez.
  - **Vigilancia:**
    - solo cuentan las lecturas;
    - la alerta no se repite;
    - una caja que nunca reportó genera su alerta sin `ultimaLectura`;
    - busca con `FilterExpression` en páginas de 25, con un tope de 40 páginas;
    - no usa el GSI, y `ListThings` va con `*` porque no admite permisos por recurso;
    - no escribe en `desired`, porque no actúa sobre un objeto apagado (la justificación está en el informe).
- **Configuración en AWS (sin secretos):** el rol, la función y la regla están preparados en `config/aws/` y en `lambda-vigilancia/README.md`. **Falta crearlos en la consola.**
- **Pruebas y valores medidos:**
  - `pio test -e pruebas`: 89 de 89;
  - `python -m unittest discover -s tests` en `lambda-vigilancia/`: 20 de 20;
  - `pio run -e caja`: compila, con 18,8 % de RAM y 73,0 % de Flash;
  - el ZIP de la vigilancia contiene solo los 5 archivos de producción.
- **Evidencias:** —
- **Cambios al contrato o al plan:**
  - 3.3: un evento por vuelta, y descarte tras 3 rechazos.
  - 6: el rol de la vigilancia no necesita el GSI.
  - 9.3: el formato del ítem de alerta, y que solo cuentan las lecturas.
- **Problemas y pendientes:**
  - Probar con Santiago en la placa: la partida completa, los eventos en el tema y la NVS.
  - En la consola:
    - aplicar `PublicarSusEventos`, porque sin ese permiso los eventos se pierden después de 3 intentos;
    - borrar el `reported.tapa` viejo;
    - crear la vigilancia.
  - Para que los eventos se guarden hace falta el PR #9 de Mateo (la tabla y la Lambda de almacenamiento).
  - `SensorLuz` usa la calibración por defecto (ADC de 0 a 4095), y Santiago tiene que fijar la real.
  - Si hay WiFi pero AWS no responde, cada intento de conexión TLS frena `loop()` varios segundos. Viene de la P3, y no afecta la prueba de cortar el WiFi.

### Iteración 3 — lun 23
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**
