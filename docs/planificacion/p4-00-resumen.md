# Planificación — Práctica 4: Caja de los Enigmas (juego, histórico y lógica de control)

La Práctica 4 ([consigna](../consigna/Práctica_4.md)) amplía la caja de la Práctica 3. La caja pasa a ser un **juego de tres desafíos**: memoria con LEDs y pulsadores, distancia con un HC-SR04 y luz con una LDR. Al superarlos, el **servo SG90** libera el seguro.

- **Alexa es el game master:** inicia la partida, da pistas, dice el progreso y cambia el modo y la dificultad.
- **AWS IoT Rules + Lambda** guardan las lecturas y los eventos en un **histórico de DynamoDB**.
- **Una Lambda programada** detecta cajas que dejaron de reportar.
- Toda la comunicación sigue pasando por el **Device Shadow**.

**Entrega: martes 24/11/2026 a las 17:45** (enlace del repositorio por el LMS-UCB). **Meta interna: 14:00.**

| Integrante | Área | Plan |
|---|---|---|
| Santiago Torres | Montaje, clases del hardware y los tres desafíos | [p4-01-santiago-hardware.md](p4-01-santiago-hardware.md) |
| Adrián Mercado | Firmware de integración, Shadow, AWS IoT y lógica de la Nube | [p4-02-adrian-firmware-aws-iot.md](p4-02-adrian-firmware-aws-iot.md) |
| Mateo Rojas | Lambda de almacenamiento, histórico de DynamoDB y backend de la skill | [p4-03-mateo-lambda.md](p4-03-mateo-lambda.md) |
| Lucía Escobar | Skill de Alexa, mockups de reportes, pruebas de voz y coordinación del informe | [p4-04-lucia-skill-informe.md](p4-04-lucia-skill-informe.md) |

Cada uno sigue en el área que tuvo en la Práctica 3 ([planificación de la P3](00-resumen.md)), ampliada con las piezas nuevas que le corresponden.

## Qué cambia respecto a la Práctica 3

| Antes (P3) | Ahora (P4) | Motivo |
|---|---|---|
| Reed switch en la tapa (`tapa` en el Shadow, `ConsultarTapaIntent`). | **Se quita.** | El grupo consideró que no aporta al juego. |
| Un actuador controlado solo por voz. | Juego de 3 desafíos en el Edge; la voz controla el modo, la dificultad y las pistas. | RF13–RF14: lógica de control automática. |
| Una tabla (`CajaEnigmasVinculaciones`). | Además, `CajaEnigmasHistorico` para lecturas, eventos y alertas. | RF11–RF12 y RF15. |
| Una Lambda (`caja-enigmas-skill`). | Además, `caja-enigmas-almacenamiento` (disparada por IoT Rules) y `caja-enigmas-vigilancia` (programada con EventBridge). | RF10 y RF13; responsabilidad única y mínimo privilegio. |

## Relación con el documento de materiales de Santiago

El plan **mantiene todo** lo que definió Santiago en su documento de materiales:
- los mismos componentes;
- el orden de los desafíos (memoria → distancia → luz);
- el cuarto pulsador solo para iniciar la partida;
- el servo, que libera el seguro al completar los tres desafíos;
- los LEDs como indicadores;
- sin buzzer.

**No se rompe nada del montaje.** Lo que se agrega es lógica alrededor de esos componentes, para cumplir requerimientos de la consigna:

| Se agrega | Por qué |
|---|---|
| Alexa también puede iniciar la partida. El cuarto pulsador sigue siendo el único botón para eso. | Alexa como game master. |
| Un modo `manual` en el que el operador abre la caja por voz sin resolver los desafíos. En modo `juego` (el normal) la caja solo se abre al resolverlos, como dice el documento. | RF5 de la P3 (abrir y cerrar por voz) se mantiene, y el §3.1 pide definir cómo conviven la voz y la lógica automática. |
| La dificultad cambia el largo de la secuencia de colores (3, 4 o 6) y la tolerancia de la distancia. | RF14: parámetros consultables y modificables por voz. |
| La distancia objetivo y el umbral de luz pasan a ser parámetros, no valores fijos en el código. | RF14. |
| La mano tiene que quedarse en la distancia unos 2 s, y la luz mantenerse 1 s. | Que no se supere un desafío por casualidad, de pasada. |
| Valores concretos de los "varios": 3 resistencias de 220 Ω (LEDs), 10 kΩ (LDR), 1 kΩ + 2 kΩ (ECHO del HC-SR04 a 3,3 V) y un capacitor de 470–1000 µF o una fuente de 5 V. | El ESP32 trabaja a 3,3 V y el servo genera picos de corriente. |

Los pines ya están aprobados en [contratos.md, sección 8](../contratos.md). Santiago puede ajustarlos al montar, actualizando el contrato, y decide si los LEDs también guían durante el desafío de distancia (rojo lejos, amarillo cerca, verde en el objetivo).

**Placas:** el prototipo usa **una sola ESP32** (`caja-enigmas-01`). `caja-enigmas-02` sigue siendo un Thing sin placa, cuyo Shadow se edita a mano para probar varios usuarios. Santiago guarda las placas. Si puede, le presta a Adrián una placa suelta **solo para desarrollo**; esa placa no forma parte del sistema que se entrega.

## Calendario común

Cada iteración termina igual para todos: **cada uno llena el registro de esa iteración** al final de su plan (ver [Registro de lo aplicado en cada iteración](#registro-de-lo-aplicado-en-cada-iteración)).

| Iteración | Meta del grupo | Cierre |
|---|---|---|
| **0** — vie 20 | **Contrato v2**: aprobado el 20/11 y publicado en [docs/contratos.md](../contratos.md). Era el único momento en que todos se esperaban. Ahora se crean las ramas y Adrián publica las interfaces. En AWS se crean la tabla, la política ampliada y los intents vacíos. | Interfaces en `main`. `main` compila sin el reed. |
| **1** — sáb 21 | Cada uno avanza con sus piezas y sus dobles de prueba (ver la tabla siguiente). | Los desafíos funcionan por Serial y `PartidaEnigmas` pasa los tests en la PC. Un mensaje del cliente MQTT de la consola queda guardado en DynamoDB. El modelo de la skill compila. |
| **2** — dom 22 | Piezas completas y primera integración. Santiago y Adrián integran el firmware en la placa. La skill se prueba completa contra el Shadow de `caja-enigmas-02`, editado a mano. Se crea la Lambda de vigilancia. | Una partida completa en la caja real. La voz cambia el modo y los parámetros en el Shadow. |
| **3** — lun 23 | Prueba de punta a punta y medición de los RNF. Evidencias en `docs/evidencias/`. | Tablas de resultados con valores medidos. |
| **4** — mar 24 | Informe armado a partir de los registros de las iteraciones 0–3, configuraciones exportadas y ensayo de la demo y la defensa. | Enlace entregado antes de las 14:00 (límite: 17:45). |

## Cómo trabajar sin bloquearse

Después del contrato del viernes, cada uno trabaja **contra el contrato, no contra el código de los demás**. Si una pieza de otro todavía no está, se usa el doble de prueba de esta tabla:

| Quién necesita… | …de quién | Mientras tanto usa |
|---|---|---|
| Adrián: los desafíos reales | Santiago | La interfaz `Desafio.h`, que Adrián publica el viernes, y desafíos falsos en `pio test -e pruebas`. |
| Santiago: la conexión con AWS | Adrián | Sus propios programas en `firmware/pruebas/` y el monitor Serial. El juego tiene que funcionar sin red, que además es un RNF. |
| Mateo: el objeto publicando | Adrián y Santiago | El **cliente de pruebas MQTT** de la consola de IoT, que publica los payloads de ejemplo del contrato. También tests con eventos falsos (patrón de [lambda/tests/apoyo.py](../../lambda/tests/apoyo.py)). |
| Mateo: los intents nuevos | Lucía | Los nombres de intents y slots fijados en el contrato v2. |
| Mateo: los textos de las pistas | Lucía | Un diccionario de pistas propio en los tests. |
| Lucía: un Shadow con datos | Adrián y Mateo | El `reported` de `caja-enigmas-02`, editado a mano en la consola. |
| Mateo y Lucía: tabla y mockups | entre ellos | La lista de **patrones de acceso** del contrato. Lucía dibuja con esa lista y Mateo diseña la tabla con la misma. |

**Cada archivo tiene un solo dueño**, indicado en la primera línea de cada plan. El único archivo compartido es `INFORME_P4.md`: Lucía crea el esqueleto con el responsable de cada sección, y cada uno edita solo sus secciones. `firmware/src/main.cpp` es de Adrián; la integración con las clases de Santiago se hace en un PR que revisan los dos juntos, con la placa.

**Puntos de integración (solo tres):**
1. **Vie 20:** el contrato v2 (ya aprobado).
2. **Dom 22:** el firmware (Santiago + Adrián).
3. **Lun 23:** la prueba de punta a punta (los cuatro).

```text
Vie: contrato v2 ──┬─► Santiago: clases + desafíos (Serial) ────────┐
                   ├─► Adrián: Shadow v2 + PartidaEnigmas (tests) ──┴─► Dom: firmware integrado ──┐
                   ├─► Mateo: tabla + almacenamiento (cliente MQTT) ──┐                           │
                   └─► Lucía: modelo + pistas + mockups ──────────────┴─► Dom: skill completa ────┴─► Lun: punta a punta
```

## Reglas de trabajo

Son las mismas de la Práctica 3 ([AGENTS.md](../../AGENTS.md)):
- Cada uno hace commits con **su propia cuenta de GitHub**, en su rama (`feat/hardware-p4`, `feat/firmware-aws-p4`, `feat/lambda-p4`, `feat/skill-p4`), y se integra a `main` con un PR revisado por otro integrante.
- Commits pequeños, con mensajes en español.
- **Nunca subir secretos.**
- Código limpio y en español, según las reglas de `AGENTS.md`.
- Si alguien necesita cambiar el contrato, lo avisa en el grupo **antes** de cambiar su código, y actualiza `docs/contratos.md` en el mismo PR.
- El informe de la P4 va en **`INFORME_P4.md`**. `INFORME.md` (P3) no se toca.
- **Al cerrar cada iteración, cada uno llena su registro** (sección siguiente) y lo sube en el mismo PR.

## Registro de lo aplicado en cada iteración

Al final de cada plan hay un bloque por iteración (0 a 3). Al cerrar la iteración, el dueño del plan escribe **lo que de verdad quedó hecho**, no lo que decía el plan; si un campo no aplica, pone "—". Son unos 10 minutos. En la iteración 4, Lucía arma `INFORME_P4.md` a partir de estos registros, sin tener que reconstruir de memoria qué se hizo, por qué y con qué resultados.

Cada campo alimenta una sección del informe que pide la consigna (§4.2):

| Campo del registro | Qué escribir | Sección del informe |
|---|---|---|
| Qué quedó funcionando | Lo que se puede demostrar, y el RF o RNF que cubre. | 3. Desarrollo e implementación |
| Archivos, commits y PR | Rutas de las clases y módulos tocados y el número de PR. | 3. Estructura del código (con enlaces) |
| Decisiones y por qué | Cada elección de diseño con su justificación: valores, ubicación Edge o Nube, permisos, frecuencias. | 2. Análisis y diseño |
| Configuración hecha | Lo creado o cambiado en la consola de AWS o de Alexa, o en el montaje, sin secretos. | 3. Configuraciones |
| Pruebas y valores medidos | Qué se probó, cuántas veces y los números obtenidos. | 4. Pruebas y 5. Resultados |
| Evidencias | Ruta de las capturas o fotos en `docs/evidencias/`, con nombres que empiecen con `p4-`. | 8. Anexos |
| Cambios al contrato o al plan | Qué se cambió respecto de lo acordado y por qué. | 1 y 2 (cambios justificados) |
| Problemas y pendientes | Qué falló, cómo se resolvió y qué quedó sin hacer. | 6. Conclusiones y 7. Recomendaciones |

## Acuerdos: contrato v2 aprobado

El grupo aprobó el contrato v2 el **20/11/2026**. La versión oficial está en **[docs/contratos.md](../contratos.md)**, y es la única fuente: si algo cambia, se cambia ahí, en el mismo PR que el código.

Lo más importante, para ubicarse rápido:

| Tema | Acuerdo | Sección del contrato |
|---|---|---|
| Shadow | Se quita `tapa`. Se agregan `modo`, `dificultad`, `distanciaObjetivoCm`, `umbralLuzPorcentaje` y `solicitudPartida` (`desired`/`reported`), y el estado de la partida y las lecturas (`reported`). | 2 |
| Dificultad | `facil`, `normal` y `dificil` fijan la secuencia (3, 4 o 6) y la tolerancia (6, 4 o 2 cm). La distancia objetivo (10–40 cm) y el umbral de luz (10–95 %) se cambian aparte. | 2 |
| Eventos | Tema `caja-enigmas/<thingName>/eventos`, con 6 tipos de evento. Cola de 20 eventos sin conexión. | 3 |
| Histórico | `CajaEnigmasHistorico` (PK `thingName`, SK `ts`), GSI disperso `alertas-por-fecha`, TTL de 30 días en las lecturas, 6 patrones de acceso. | 5 |
| Lambdas y reglas | `caja-enigmas-almacenamiento` (reglas `CajaEnigmasLecturas` y `CajaEnigmasEventos`) y `caja-enigmas-vigilancia` (EventBridge cada 5 min). | 6 |
| Skill | 8 intents nuevos; se quita `ConsultarTapaIntent`; tipos `TipoModo`, `TipoDificultad` y `TipoParametro`. | 7 |
| Hardware | Pines, divisor del ECHO, LDR en ADC1 y alimentación. | 8 |
| Modos | `juego`: solo el juego mueve la cerradura. `manual`: solo la voz. | 9.1 |
| Lógica de control | Partida en el Edge; caja sin reportar (15 min) en la Nube. | 9.2 y 9.3 |
| Firmware | Interfaz `Desafio` y `ParametrosJuego`; ningún desafío bloquea `loop()`. | 10 |


## Riesgos y cómo se cubren

| Riesgo | Qué hacer | Responsable |
|---|---|---|
| La consigna **restringe el ultrasónico**: "solo si el caso de uso lo requiere". | Justificar en el informe que la mecánica del desafío es medir **sin contacto** la distancia de la mano a un objetivo en cm, con tolerancia, y comparar con alternativas (sensor IR de obstáculo: solo dice "hay algo", no cuánto). | Santiago (técnico) y Lucía (caso de uso) |
| Servo, HC-SR04 y LEDs desde VIN pueden reiniciar el ESP32 por picos de corriente. | Fuente de 5 V externa o, como mínimo, un capacitor de 470–1000 µF. Medirlo en las pruebas de estabilidad. | Santiago |
| Un `delay()` o un `pulseIn` sin límite en un desafío congela MQTT. | Todos los desafíos son no bloqueantes, con `millis()` y un `pulseIn` de unos 25 ms como máximo. Regla del contrato de `Desafio.h`. | Santiago y Adrián |
| Alexa no recibe avisos del objeto: no puede "anunciar" que se superó un desafío. | El game master responde cuando se le pregunta ("¿cómo voy?"). Se explica en el informe. | Lucía |
| El plazo es de 4 días. | Contrato aprobado desde el primer día, dobles de prueba y meta de entrega a las 14:00 del martes. | Todos |

## Trazabilidad: quién cubre cada requerimiento

| Requerimiento | Responsable principal | Apoyo |
|---|---|---|
| RF1–RF3 (objeto, reporte y `delta`), sin la tapa | Adrián | Santiago |
| RF4–RF9 (voz y vinculación) | Mateo | Lucía |
| RF10 (reglas de IoT) | Mateo | Adrián (política y temas) |
| RF11 (validar y guardar lecturas) | Mateo | — |
| RF12 (eventos de actuadores con `origen`) | Adrián (los emite) | Mateo (los guarda) |
| RF13 Edge (juego) | Adrián (`PartidaEnigmas`) | Santiago (desafíos) |
| RF13 Nube (caja sin reportar) | Adrián | Mateo (tabla) |
| RF14 (modo y parámetros por voz) | Mateo (handlers) | Adrián (Shadow) y Lucía (modelo) |
| RF15 (mockups) | Lucía | Mateo (que el modelo de datos los permita) |
| RNF integridad y latencia del almacenamiento | Mateo | — |
| RNF tiempo de respuesta, Edge | Santiago | Adrián |
| RNF tiempo de respuesta, Nube | Adrián | — |
| RNF operación sin conexión y reconexión | Adrián | Santiago |
| RNF costo | Lucía | Adrián (mensajes), Mateo (invocaciones y escrituras) |
| RNF exactitud de medición (HC-SR04 y LDR) | Santiago | — |
| Seguridad: permisos justificados de cada rol | El dueño de cada recurso | Adrián revisa |
