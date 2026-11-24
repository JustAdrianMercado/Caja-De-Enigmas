# Contratos entre componentes — versión 2 (Práctica 4)

Este documento fija los nombres, formatos y reglas que comparten el firmware, las Lambdas y la skill, para que cada integrante trabaje en paralelo. **Si alguien necesita cambiar algo de aquí, lo avisa en el grupo antes de cambiar su código** y actualiza este archivo en el mismo PR.

**Versión 2, aprobada por el grupo el 20/11/2026** para la Práctica 4 ([planificación](planificacion/p4-00-resumen.md)).
- La versión 1 (Práctica 3) está en el historial de git (commit `176abf6`) y la describe [INFORME.md](../INFORME.md).
- El código y la configuración de AWS se adaptan a esta versión durante las iteraciones de la P4. Hasta que cada PR se integre, `main` puede seguir funcionando con la v1.
- La numeración de las secciones 1 a 8 es la de la v1, porque el código la cita (por ejemplo, "contratos.md, sección 2").

**Cambios respecto de la v1:**
- Se quita el reed switch: el atributo `tapa` y `ConsultarTapaIntent`.
- La caja pasa a ser un juego de tres desafíos con modo y parámetros.
- Se agregan el tema de eventos, la tabla de histórico, dos Lambdas nuevas, dos reglas de IoT y una regla programada.

## 1. AWS

| Elemento | Valor |
|---|---|
| Región | `us-east-1` (N. Virginia), para todo: IoT Core, Lambda, DynamoDB y EventBridge. |
| Cuenta | Una sola cuenta del grupo, con un usuario IAM por integrante. Nadie usa la cuenta raíz ni crea access keys. |
| Nombre de los Things | `caja-enigmas-NN`, con dos dígitos. `caja-enigmas-01` es la caja real; `caja-enigmas-02` es un Thing sin placa, cuyo Shadow se edita a mano para probar varios usuarios. |
| Política de IoT | `CajaEnigmasPolitica` ([config/aws/politica-iot.json](../config/aws/politica-iot.json)), asociada al certificado de cada Thing. **v2:** además de los temas del Shadow, permite `iot:Publish` en `caja-enigmas/${iot:Connection.Thing.ThingName}/eventos`. |
| Client id MQTT | Igual al nombre del Thing (la política lo exige). |
| Endpoint de datos | `xxxxxxxx-ats.iot.us-east-1.amazonaws.com` (IoT Core → Settings). Se guarda en la configuración local del firmware y en la variable `IOT_ENDPOINT` de la Lambda de la skill, no en el código. |

## 2. Documento Shadow (Shadow clásico, sin nombre)

| Atributo | Sección | Tipo | Valores válidos | Por defecto | Quién lo escribe |
|---|---|---|---|---|---|
| `cerradura` | `desired` / `reported` | string | `"abierta"`, `"cerrada"` | `"cerrada"` | Lambda de la skill, solo en modo `manual` / ESP32 |
| `modo` | `desired` / `reported` | string | `"juego"`, `"manual"` | `"juego"` | Lambda de la skill / ESP32 |
| `dificultad` | `desired` / `reported` | string | `"facil"`, `"normal"`, `"dificil"` | `"normal"` | Lambda de la skill / ESP32 |
| `distanciaObjetivoCm` | `desired` / `reported` | entero | 10–40 | 20 (provisional) | Lambda de la skill / ESP32 |
| `umbralLuzPorcentaje` | `desired` / `reported` | entero | 10–95 | 70 (provisional) | Lambda de la skill / ESP32 |
| `solicitudPartida` | `desired` / `reported` | entero | ≥ 0 | 0 | Lambda de la skill (la incrementa) / ESP32 (la adopta) |
| `longitudSecuencia` | `reported` | entero | 3, 4 o 6 | según la dificultad | ESP32 |
| `toleranciaCm` | `reported` | entero | 6, 4 o 2 | según la dificultad | ESP32 |
| `estadoPartida` | `reported` | string | `"esperando"`, `"en_curso"`, `"resuelta"` | `"esperando"` | ESP32 |
| `desafioActual` | `reported` | string | `"memoria"`, `"distancia"`, `"luz"`, `"ninguno"` | `"ninguno"` | ESP32 |
| `idPartida` | `reported` | entero | ≥ 0 | 0 | ESP32 (contador local en NVS) |
| `distanciaCm` | `reported` | entero | 2–400 | se omite si no hay lectura válida | ESP32 |
| `luzPorcentaje` | `reported` | entero | 0–100 | — | ESP32 |
| `codigoVinculacion` | `reported` | string de 4 dígitos | `"0000"`–`"9999"` | generado en el primer arranque | ESP32 |
| `firmware` | `reported` | string | versión semántica, p. ej. `"0.3.0"` | — | ESP32 |

Los valores por defecto de `distanciaObjetivoCm` y `umbralLuzPorcentaje` son provisionales: se calibran en la iteración 1 con el montaje real y se actualizan aquí.

**Dificultad.** `dificultad` fija los dos valores que miden cuán difícil es el juego. Esos valores solo se reportan; no se escriben en `desired`.

| `dificultad` | `longitudSecuencia` | `toleranciaCm` |
|---|---|---|
| `facil` | 3 | 6 |
| `normal` | 4 | 4 |
| `dificil` | 6 | 2 |

`distanciaObjetivoCm` y `umbralLuzPorcentaje` son parámetros de calibración que se cambian por separado. Así la dificultad y los parámetros no se pisan.

**`solicitudPartida`** es un contador para que la orden "iniciar partida" sea idempotente.
- La skill escribe `desired.solicitudPartida = reported.solicitudPartida + 1`.
- Cuando el ESP32 recibe un valor distinto del suyo, lo adopta y lo reporta. Si está en modo `juego`, además inicia una partida.
- Un mismo valor repetido no inicia dos partidas. Una orden escrita como texto ("iniciar") quedaría en el `delta` para siempre.

Ejemplo con una orden pendiente:

```json
{
  "state": {
    "desired":  { "modo": "juego", "dificultad": "dificil", "solicitudPartida": 8 },
    "reported": { "cerradura": "cerrada", "modo": "juego", "dificultad": "normal",
                  "distanciaObjetivoCm": 20, "umbralLuzPorcentaje": 70,
                  "solicitudPartida": 7, "longitudSecuencia": 4, "toleranciaCm": 4,
                  "estadoPartida": "esperando", "desafioActual": "ninguno", "idPartida": 11,
                  "distanciaCm": 63, "luzPorcentaje": 22,
                  "codigoVinculacion": "4821", "firmware": "0.3.0" },
    "delta":    { "dificultad": "dificil", "solicitudPartida": 8 }
  }
}
```

Reglas:
- **Solo la Lambda de la skill escribe en `desired`** y **solo el ESP32 escribe en `reported`**.
- El ESP32 ignora en el `delta` los atributos que no conoce y los valores fuera de la lista o del rango. Si recibe un valor inválido, no cambia nada y vuelve a reportar su estado real.
- Después de aplicar un `delta`, el ESP32 reporta el nuevo estado. Así `reported` vuelve a coincidir con `desired` y AWS borra el `delta`.
- El ESP32 nunca borra `desired`.
- **En modo `juego`, el ESP32 ignora `cerradura` en el `delta`.** Además, al pasar a modo `juego` la skill pone `desired.cerradura` en `null` en la misma actualización. Si no lo hiciera, una orden vieja de "abrir" dada en modo manual quedaría en el `delta` y abriría la caja en medio de una partida.
- El modo, la dificultad, los parámetros, `solicitudPartida` e `idPartida` se guardan en NVS: después de un reinicio sin red, el juego sigue configurado igual.

## 3. Temas MQTT

### 3.1 Shadow (sin cambios respecto de la v1)

Prefijo: `$aws/things/<thingName>/shadow/`

| Tema | Dirección | Uso |
|---|---|---|
| `update` | ESP32 → AWS | Publica `{"state":{"reported":{...}}}`. |
| `update/delta` | AWS → ESP32 | Recibe los cambios de `desired` que difieren de `reported`. |
| `get` | ESP32 → AWS | Al conectar o reconectar pide el Shadow completo (payload vacío). |
| `get/accepted` | AWS → ESP32 | Respuesta al `get`: si trae `state.delta`, se aplica (orden pendiente). |
| `update/rejected`, `get/rejected` | AWS → ESP32 | Opcionales: registrar errores por Serial. |

### 3.2 Eventos (nuevo)

Tema: `caja-enigmas/<thingName>/eventos`. Lo publican el ESP32 y, solo para las pistas, la Lambda de la skill. Un mensaje es un evento, en JSON plano:

```json
{ "tipo": "desafio_superado", "idPartida": 12, "desafio": "memoria",
  "duracionMs": 18450, "intentos": 2, "tsObjeto": "2026-11-21T15:20:03.120Z" }
```

| `tipo` | Lo publica | Campos obligatorios, además de `tipo` |
|---|---|---|
| `partida_iniciada` | ESP32 | `idPartida`, `origen` (`edge` = pulsador, `voz` = `solicitudPartida`), `dificultad` |
| `desafio_superado` | ESP32 | `idPartida`, `desafio`, `duracionMs`, `intentos` |
| `desafio_fallado` | ESP32 | `idPartida`, `desafio` |
| `partida_resuelta` | ESP32 | `idPartida`, `duracionMs` (de toda la partida) |
| `cerradura` | ESP32 | `cerradura` (`abierta` o `cerrada`), `origen` (`voz`, `edge` o `nube`) |
| `pista` | Lambda de la skill | `desafio`, `origen` (`voz`) |

| Campo | Tipo | Valores válidos |
|---|---|---|
| `idPartida` | entero | ≥ 1 |
| `desafio` | string | `memoria`, `distancia`, `luz` |
| `duracionMs` | entero | 0–3 600 000 |
| `intentos` | entero | 1–99 |
| `origen` | string | `voz`, `edge`, `nube` |
| `dificultad` | string | `facil`, `normal`, `dificil` |
| `tsObjeto` | string, ISO-8601 UTC con ms | Hora del ESP32. Se omite si todavía no hay hora NTP (por ejemplo, arrancó sin red). |

- **`duracionMs` de `desafio_superado`** cuenta desde el primer intento de ese desafío, con los reintentos incluidos. `intentos` es 1 más la cantidad de fallos.
- El ESP32 **satura** `duracionMs` en 3 600 000 e `intentos` en 99. Así un evento de una partida muy larga no queda fuera del rango y no se descarta.
- **`origen` de la cerradura:**
  - `voz` cuando el servo se mueve por un `delta`, porque solo la skill escribe `desired.cerradura`;
  - `edge` cuando lo mueve el juego: cierra al iniciar una partida y abre al resolverla;
  - `nube` queda reservado para una regla futura.
- El `thingName` **no** va en el payload. La regla lo toma del tema, y la política de IoT impide que una caja publique en el tema de otra.

### 3.3 Frecuencia de reporte

- **Por evento:** cada cambio de `cerradura`, `estadoPartida`, `desafioActual`, `modo`, `dificultad` o parámetros se reporta en el Shadow en ≤ 1 s. Cada evento de la tabla 3.2 se publica en el momento.
- **Periódico de respaldo:** reporte completo del Shadow cada **60 s**, con las lecturas `distanciaCm` y `luzPorcentaje`. Las lecturas también se reportan en cada cambio de desafío.
- **Sin conexión:**
  - el juego sigue funcionando;
  - los eventos se guardan en una cola de **20 eventos** y se publican en orden al reconectar, uno por vuelta de `loop()`; si la cola se llena, se descarta el más viejo;
  - si AWS rechaza el mismo evento 3 veces con la caja conectada (por ejemplo, porque falta el permiso en la política), se descarta y se cuenta, para que no trabe la cola;
  - las lecturas sin conexión no se guardan, porque solo interesa la actual.
- **Justificación:** el juego produce eventos discretos que se reportan de inmediato. El respaldo de 60 s mantiene el Shadow vigente y da una serie de lecturas con unos 1 440 mensajes por día y por caja. La estimación de costo completa va en el informe.

## 4. Vinculación usuario ↔ caja (sin cambios respecto de la v1)

1. El ESP32 muestra su código de 4 dígitos por Serial al iniciar y lo publica en `reported.codigoVinculacion`.
2. El usuario dice: *"vincula mi caja **uno** con el código **cuatro ocho dos uno**"*.
3. La Lambda arma `thingName = "caja-enigmas-" + f"{numeroCaja:02d}"`.
4. Valida que el Thing exista (`DescribeThing`); si no existe, lo informa y no guarda nada.
5. Lee el Shadow (`GetThingShadow`) y compara `codigo` con `reported.codigoVinculacion`; si no coinciden, lo informa y no guarda nada.
6. Guarda o reemplaza la vinculación (`PutItem`). Una vinculación nueva **reemplaza** a la anterior (cambiar de caja).
7. Desvincular hace `DeleteItem`.

Un usuario tiene como máximo una caja vinculada.

## 5. DynamoDB

### 5.1 `CajaEnigmasVinculaciones` (sin cambios respecto de la v1)

| Elemento | Valor |
|---|---|
| Capacidad | *On-demand*. |
| Clave de partición | `userId` (S): `context.System.user.userId` de la solicitud de Alexa. |
| Atributos | `thingName` (S), `fechaVinculacion` (S, ISO-8601 UTC, p. ej. `2026-11-08T15:20:00Z`). |

### 5.2 `CajaEnigmasHistorico` (nueva)

Guarda las lecturas, los eventos y las alertas de todas las cajas. Capacidad *on-demand*.

| Elemento | Valor |
|---|---|
| Clave de partición | `thingName` (S). |
| Clave de ordenamiento | `ts` (S): `<ISO-8601 UTC con ms>#<tipo o evento>`, p. ej. `2026-11-21T15:20:03.120Z#desafio_superado`. El sufijo evita que dos ítems del mismo milisegundo se pisen, y el prefijo de fecha permite consultar por rango. |
| `tipo` (S) | `lectura`, `evento` o `alerta`. |
| `evento` (S) | Solo en los eventos: el `tipo` del mensaje de la sección 3.2. |
| Campos del mensaje | Los de la sección 3.2, o `distanciaCm` y `luzPorcentaje` en las lecturas. |
| `tsObjeto` (S) | Si vino en el mensaje. |
| `tsRegla` (N) | Epoch en ms, de la función `timestamp()` de la regla. |
| `tsGuardado` (N) | Epoch en ms, en el momento del `PutItem`. Sirve para medir la latencia del almacenamiento. |
| `expiraEn` (N) | Epoch en s. **Solo en las lecturas**, a 30 días. Es el atributo de TTL de la tabla. |
| GSI `alertas-por-fecha` | Clave de partición `categoriaAlerta` (S) y de ordenamiento `ts` (S). Es **disperso**: solo los ítems de alerta tienen `categoriaAlerta` (`sin_reporte`). |

Para `ts` se usa `tsObjeto` si vino en el mensaje y, si no, `tsRegla`.

**Patrones de acceso** (los mockups de reportes y la lógica de la Nube usan solo estos):

| # | Patrón | Cómo se resuelve |
|---|---|---|
| 1 | Eventos de una caja en un rango de fechas (historial de partidas) | `Query` por `thingName` con `ts BETWEEN`, filtrando `tipo = evento`. |
| 2 | Partidas iniciadas y resueltas de una caja en el día | Igual que el 1, con el rango del día y filtrando `evento`. |
| 3 | Fallos, intentos y duración de cada desafío en un rango de fechas | Igual que el 1, con `desafio_superado` y `desafio_fallado`, y agrupando por `desafio`. |
| 4 | Pistas pedidas por desafío | Igual que el 1, con `evento = pista`. |
| 5 | Última lectura de una caja (vigilancia) | `Query` por `thingName` con `ScanIndexForward = false`, leyendo los últimos ítems hasta encontrar una lectura. |
| 6 | Alertas recientes de todas las cajas | `Query` sobre el GSI `alertas-por-fecha` con `categoriaAlerta = sin_reporte`, `ScanIndexForward = false`. |

## 6. Lambdas, reglas de AWS IoT y regla programada

| Función | Carpeta | Disparador | Responsabilidad | Rol y permisos mínimos | Variables de entorno |
|---|---|---|---|---|---|
| `caja-enigmas-skill` | `lambda/` | Alexa Skills Kit, con verificación del Skill ID | Backend de la skill. | `caja-enigmas-lambda-rol` ([política](../config/aws/politica-rol-lambda.json)): los de la v1, más `iot:Publish` sobre `caja-enigmas/*/eventos` (pistas). | `TABLA_VINCULACIONES=CajaEnigmasVinculaciones`, `PREFIJO_THING=caja-enigmas-`, `IOT_ENDPOINT`. |
| `caja-enigmas-almacenamiento` | `lambda-almacenamiento/` | Reglas `CajaEnigmasLecturas` y `CajaEnigmasEventos` | Valida tipos y rangos (secciones 2 y 3.2), descarta los mensajes inválidos registrando el motivo, enriquece (`thingName`, `ts`, `tsGuardado`, `expiraEn`) y guarda. | `caja-enigmas-almacenamiento-rol`: `dynamodb:PutItem` sobre `CajaEnigmasHistorico` y los logs. | `TABLA_HISTORICO=CajaEnigmasHistorico`, `DIAS_RETENCION_LECTURAS=30`. |
| `caja-enigmas-vigilancia` | `lambda-vigilancia/` | Regla programada de EventBridge `caja-enigmas-vigilancia-cada-5-min`, `rate(5 minutes)` | Lógica de la Nube: detecta las cajas sin reportar (sección 9.3). | `caja-enigmas-vigilancia-rol` ([política](../config/aws/politica-rol-vigilancia.json)): `iot:ListThings`, y `dynamodb:Query` y `dynamodb:PutItem` sobre `CajaEnigmasHistorico`. No usa el GSI. | `TABLA_HISTORICO=CajaEnigmasHistorico`, `PREFIJO_THING=caja-enigmas-`, `MINUTOS_SIN_REPORTE=15`. |

Todas usan Python 3.12 en us-east-1.

**Reglas de AWS IoT** (SQL versión `2016-03-23`; acción: invocar `caja-enigmas-almacenamiento`):

```sql
-- CajaEnigmasLecturas: lecturas de los sensores que llegan en el reporte del Shadow.
SELECT state.reported.distanciaCm AS distanciaCm,
       state.reported.luzPorcentaje AS luzPorcentaje,
       'lectura' AS tipo, topic(3) AS thingName, timestamp() AS tsRegla
FROM '$aws/things/+/shadow/update/accepted'
WHERE NOT isUndefined(state.reported.distanciaCm)
   OR NOT isUndefined(state.reported.luzPorcentaje)

-- CajaEnigmasEventos: eventos del juego, de la cerradura y de las pistas.
SELECT *, 'evento' AS tipoMensaje, topic(2) AS thingName, timestamp() AS tsRegla
FROM 'caja-enigmas/+/eventos'
```

- La regla de eventos agrega `tipoMensaje` y no `tipo`, porque `tipo` ya viene en el payload con el nombre del evento.
- Ninguna de las dos reglas usa un rol IAM. Para invocar la Lambda, AWS IoT usa el permiso basado en recursos de la función, que la consola agrega al crear la acción.
- Lo mismo vale para la regla programada de EventBridge.
- Las definiciones exportadas van en `config/aws/reglas/` y las políticas de los roles en `config/aws/`.

## 7. Skill de Alexa

| Elemento | Valor |
|---|---|
| Idioma | Spanish (US), `es-US`. |
| Nombre de invocación | `caja de enigmas` |
| Modelo exportado | `config/alexa/modelo-interaccion.es-US.json` |

| Intent | Slots | Efecto |
|---|---|---|
| `VincularCajaIntent` | `numeroCaja` (`AMAZON.NUMBER`), `codigo` (`AMAZON.FOUR_DIGIT_NUMBER`) | Vincula o cambia la caja (RF7, RF9). |
| `DesvincularCajaIntent` | — | Elimina la vinculación (RF9). |
| `ConsultarCerraduraIntent` | — | Lee `reported.cerradura` (RF6). |
| `AbrirCerraduraIntent` | — | `desired.cerradura = "abierta"` (RF5). **Solo en modo `manual`**; en modo `juego` se rechaza (sección 9.1). |
| `CerrarCerraduraIntent` | — | `desired.cerradura = "cerrada"` (RF5). **Solo en modo `manual`**; en modo `juego` se rechaza. |
| `IniciarPartidaIntent` | — | Incrementa `desired.solicitudPartida`. En modo `manual` se rechaza. |
| `PedirPistaIntent` | — | Lee `reported.desafioActual` e `idPartida`. Entrega las tres pistas de `lambda/pistas.py` en orden y luego repite la tercera. Reinicia la secuencia al cambiar la partida, el desafío o la sesión. Publica un evento `pista` por cada pista entregada. Sin una partida en curso, lo explica. |
| `ConsultarProgresoIntent` | — | Responde con `estadoPartida` y `desafioActual`. |
| `CambiarModoIntent` | `modo` (`TipoModo`) | `desired.modo`. Al pasar a `juego`, además `desired.cerradura = null` (sección 2). |
| `ConsultarModoIntent` | — | Lee `reported.modo`. |
| `CambiarDificultadIntent` | `dificultad` (`TipoDificultad`) | `desired.dificultad`. |
| `CambiarParametroIntent` | `parametro` (`TipoParametro`), `valor` (`AMAZON.NUMBER`) | Valida el rango de la sección 2. Si es válido, escribe en `desired`; si no, explica el rango y no escribe nada. |
| `ConsultarParametrosIntent` | — | La dificultad, `longitudSecuencia`, `toleranciaCm`, `distanciaObjetivoCm` y `umbralLuzPorcentaje` reportados. |
| `ConsultarSensoresIntent` | — | Lee las mediciones `reported.distanciaCm` y `reported.luzPorcentaje` de la caja vinculada. Informa cada lectura válida y avisa si alguna o ambas faltan o son inválidas. No escribe en el Shadow. |
| `AMAZON.HelpIntent`, `CancelIntent`, `StopIntent`, `FallbackIntent`, `NavigateHomeIntent` | — | Intents integrados. |

`ConsultarTapaIntent` **se quita**.

**Tipos de slot propios.** El backend usa el **ID** de la resolución de entidades, que coincide con el valor del Shadow, y nunca el texto que dijo el usuario.

| Tipo | ID | Valor y sinónimos |
|---|---|---|
| `TipoModo` | `juego` | juego; jugar, automático |
| `TipoModo` | `manual` | manual; operador |
| `TipoDificultad` | `facil` | fácil; sencillo |
| `TipoDificultad` | `normal` | normal; medio |
| `TipoDificultad` | `dificil` | difícil; complicado |
| `TipoParametro` | `distanciaObjetivoCm` | distancia objetivo; distancia |
| `TipoParametro` | `umbralLuzPorcentaje` | umbral de luz; luz |

Lucía puede agregar sinónimos sin cambiar este contrato; los IDs no se cambian.

Todo intent que actúa sobre una caja resuelve primero la vinculación del `userId`. Si no la hay, la skill explica cómo vincular (RF8).

## 8. Hardware

Los valores del firmware están todos en [firmware/include/ConfiguracionHardware.h](../firmware/include/ConfiguracionHardware.h); al calibrar el montaje se cambia solo ese archivo. El cableado está en [docs/GUIA_MONTAJE.md](GUIA_MONTAJE.md).

| Elemento | Valor | Nota |
|---|---|---|
| Placa | ESP32 DOIT DEVKIT V1 (`esp32doit-devkit-v1`). | Una sola placa en el prototipo. |
| Servo SG90 (señal) | GPIO 18. | Igual que en la v1. Ángulos provisionales: cerrada = 45°, abierta = 135°; calibrar con `firmware/pruebas/servo`. |
| LEDs rojo, amarillo y verde | GPIO 25, 26 y 27. | Cada uno con una resistencia de 220 Ω. |
| Pulsadores rojo, amarillo, verde e inicio | GPIO 13, 14, 16 y 17. | Entre el pin y GND, con `INPUT_PULLUP`: `LOW` = pulsado. Antirrebote de 50 ms. |
| HC-SR04 | TRIG en GPIO 23, ECHO en GPIO 19. | Se alimenta con 5 V. **El ECHO sale a 5 V: divisor de 1 kΩ y 2 kΩ** para llevarlo a 3,3 V. |
| LDR | GPIO 34. | Divisor con 10 kΩ. **Tiene que ir en ADC1 (GPIO 32–39)**, porque ADC2 no funciona con el WiFi encendido. |
| Alimentación | Fuente externa de 5 V con **GND común con el ESP32**, o como mínimo un capacitor de 470–1000 µF en los 5 V del servo. | El servo y el HC-SR04 generan picos de corriente que pueden reiniciar el ESP32 si se alimentan solo desde VIN. |
| Reed switch | **Se quita** en la v2. | — |

Santiago puede ajustar los pines al montar, actualizando esta tabla y `ConfiguracionHardware.h` en el mismo PR.

## 9. Modos de operación y lógica de control

### 9.1 Convivencia de la voz con la lógica automática

El servo lo pueden mover el juego (al resolver los desafíos) y la voz (RF5). Para que no se contradigan, hay dos modos:

| | Modo `juego` (por defecto) | Modo `manual` (operador) |
|---|---|---|
| Para qué | Jugar. | Guardar el premio, probar o reiniciar la caja. |
| Quién mueve la cerradura | Solo el juego: cierra al iniciar una partida y abre al resolverla. | Solo la voz. |
| "Abre / cierra la caja" | Se rechaza con una explicación, p. ej. *"La caja está en modo juego: hay que resolver los desafíos o pasarla a modo manual"*. No escribe nada en el Shadow. | Se aplica. |
| "Inicia una partida" y pulsador de inicio | Inician una partida. | Se rechaza por voz; el pulsador no hace nada. |
| Pistas, progreso y parámetros por voz | Disponibles. | Disponibles. |

**Al cambiar de modo:**
- **A `manual`:** se detiene la partida en curso, que queda abandonada (`estadoPartida = "esperando"`, `desafioActual = "ninguno"`). No se publica ningún evento. Una partida ya resuelta también vuelve a `esperando`.
- **A `juego`:** la cerradura queda como estaba hasta que se inicie una partida. La skill pone `desired.cerradura` en `null` (sección 2).

### 9.2 Partida (lógica del Edge)

```text
esperando ──(pulsador de inicio o solicitudPartida nueva, en modo juego)──► memoria
memoria ──superado──► distancia ──superado──► luz ──superado──► resuelta
```

- **Al iniciar:**
  - se incrementa `idPartida` y el servo cierra la cerradura (evento `cerradura` con origen `edge`);
  - se publica `partida_iniciada`;
  - pulsar inicio con una partida en curso la reinicia como partida nueva.
- **Cada desafío superado** publica `desafio_superado`, con su duración y sus intentos.
- **Un fallo** (solo en memoria) publica `desafio_fallado` y reinicia ese mismo desafío.
- **Al superar la luz**, el servo abre la cerradura (evento `cerradura` con origen `edge`) y se publica `partida_resuelta`. El estado queda `resuelta` hasta que se inicie otra partida.

| Desafío | Se supera cuando | Falla cuando |
|---|---|---|
| Memoria | El jugador repite con los pulsadores la secuencia de `longitudSecuencia` colores que muestran los LEDs. | Pulsa un color equivocado. |
| Distancia | La mano se mantiene a `distanciaObjetivoCm ± toleranciaCm` durante **2 s** seguidos. | No falla. |
| Luz | `luzPorcentaje ≥ umbralLuzPorcentaje` durante **1 s** seguido. | No falla. |

### 9.3 Ubicación de cada regla

| Regla | Dónde | Justificación |
|---|---|---|
| Partida: validación de los desafíos, avance y apertura del servo (9.2) | Edge (ESP32) | Necesita respuesta inmediata y tiene que funcionar sin Internet. |
| Caja sin reportar: si la última lectura de una caja tiene más de `MINUTOS_SIN_REPORTE` (15 min), guardar una alerta | Nube (`caja-enigmas-vigilancia`, cada 5 min) | El objeto no puede avisar de su propia desconexión, y se necesita el histórico. |

**Detalles de la regla de la Nube:**
- La Lambda recorre los Things con el prefijo `caja-enigmas-` (`iot:ListThings`) y busca la última lectura de cada uno (patrón 5). Solo cuentan las lecturas: un evento, como una pista pedida por voz, no prueba que la caja esté encendida.
- La alerta es un ítem con:
  - `ts = "<ISO-8601 UTC con ms>#alerta"`, `tipo = alerta` y `categoriaAlerta = sin_reporte`;
  - `tsGuardado` (epoch en ms);
  - `ultimaLectura` (ISO) y `minutosSinReporte`, que se omiten si la caja nunca reportó.
- No se repite: si lo más nuevo de la caja, entre lecturas y alertas, ya es una alerta, no se guarda otra hasta que la caja vuelva a reportar.
- `caja-enigmas-02` no tiene placa, así que genera su alerta. Sirve para mostrar la regla en la demo.
- En el peor caso, la alerta aparece a los 15 + 5 = 20 min.
- **Para la demo** se puede bajar `MINUTOS_SIN_REPORTE` (p. ej. a 3) y la frecuencia de la regla programada (p. ej. a 1 min) sin tocar el código. Lo que se use en la demo se documenta en el informe.

## 10. Interfaz entre la partida y los desafíos (firmware)

`PartidaEnigmas` (Adrián) encadena los desafíos sin saber qué hardware usan. Cada desafío (Santiago) implementa esta interfaz:

```cpp
enum class ResultadoDesafio : uint8_t { EnCurso, Superado, Fallado };

class Desafio {
public:
    virtual ~Desafio() = default;
    // Empieza de cero con los parámetros vigentes.
    virtual void iniciar(const ParametrosJuego& parametros) = 0;
    // Se llama en cada vuelta de loop(); nunca bloquea.
    virtual void actualizar() = 0;
    // Al abandonar la partida (paso a modo manual o reinicio): apaga sus LEDs y deja de medir.
    virtual void detener() = 0;
    virtual ResultadoDesafio resultado() const = 0;
    // "memoria", "distancia" o "luz", igual que en el Shadow y los eventos.
    virtual const char* nombre() const = 0;
};

struct ParametrosJuego {
    uint8_t longitudSecuencia;
    uint8_t toleranciaCm;
    uint8_t distanciaObjetivoCm;
    uint8_t umbralLuzPorcentaje;
};
```

Reglas:
- **Ningún desafío bloquea `loop()`.** Nada de `delay()`: los tiempos se manejan con `millis()`. `pulseIn` del HC-SR04 tiene un límite de unos 25 ms, y se mide como máximo cada 100 ms.
- Cada desafío maneja sus propios LEDs. Si la partida se abandona a mitad de un desafío, `PartidaEnigmas` llama a `detener()` para que no quede ningún LED encendido.
- `Superado` y `Fallado` son finales: el resultado no cambia hasta el próximo `iniciar()`.
- Después de `Fallado`, `PartidaEnigmas` cuenta el intento y vuelve a llamar a `iniciar()`.
- `iniciar()` copia los parámetros que necesita. Si cambian por voz durante la partida, se aplican recién en el próximo `iniciar()`.
- `ParametrosJuego` y la tabla de dificultades son lógica pura, sin Arduino, para probarlas en la PC (`pio test -e pruebas`). La persistencia en NVS va en una clase aparte.
- El código de referencia está en [firmware/include/Desafio.h](../firmware/include/Desafio.h) y [firmware/include/ParametrosJuego.h](../firmware/include/ParametrosJuego.h).
