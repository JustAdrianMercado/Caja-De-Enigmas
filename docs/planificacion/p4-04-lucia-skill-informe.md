# Plan de Lucía Escobar — Skill de Alexa, mockups, pruebas de voz y coordinación del informe

**Rama:** `feat/skill-p4` · **Archivos a tu cargo:**
- `config/alexa/modelo-interaccion.es-US.json` y `config/alexa/README.md`;
- `lambda/pistas.py`;
- `docs/mockups/**`;
- coordinación de `INFORME_P4.md`.

Ver los acuerdos en [p4-00-resumen.md](p4-00-resumen.md). Al cerrar cada iteración, llenás tu [registro](#registro-de-lo-aplicado) al final de este archivo.

Sos la responsable de lo que el jugador escucha y de lo que el dueño del escape room ve:
- la skill como game master: intents, slots, frases y pistas;
- los mockups de reportes (RF15);
- las pruebas de todas las frases.

También coordinás el informe, que es 1/3 de la nota.

No necesitás la caja para trabajar. El simulador de la consola de Alexa resuelve las frases sin backend, y para respuestas reales se edita a mano el `reported` de `caja-enigmas-02` en la consola de IoT.

## Iteración 0 — vie 20: nombres y esqueleto del informe

1. Los nombres de los intents, los slots y los tipos propios (con sus IDs) ya están aprobados en [contratos.md, sección 7](../contratos.md). Los patrones de acceso de tus mockups están en la sección 5.2. Podés agregar sinónimos sin cambiar el contrato; los IDs no se cambian.
2. En la consola de Alexa, crear los intents nuevos con 2 o 3 frases cada uno y **quitar `ConsultarTapaIntent`**.
3. **`INFORME_P4.md`** con las secciones del §4.2 de la consigna y el responsable de cada una, según "Secciones del informe" más abajo.

**Listo cuando:** los intents nuevos existen en la consola y el esqueleto del informe está en `main`.

## Iteración 1 — sáb 21: modelo de interacción y pistas

- **Frases:** al menos 6 por intent nuevo, con variantes naturales.
  - "empecemos a jugar", "quiero una pista", "¿cómo voy?", "¿en qué desafío estoy?";
  - "pon la caja en modo manual", "sube la dificultad a difícil";
  - "cambia la distancia objetivo a veinte centímetros", "¿cuáles son los parámetros del juego?".
- **Sinónimos** en los tipos propios: "fácil", "facil", "sencillo"; "modo juego", "modo automático"; etc. El backend tiene que recibir siempre el valor del contrato (`facil`, `juego`…), no el sinónimo.
- **Compilar** el modelo y probar cada frase con el *Utterance Profiler*: tiene que resolver el intent y el slot correctos. Exportar el JSON a `config/alexa/`.
- **`lambda/pistas.py`**: solo datos, sin lógica. Un diccionario por desafío (`memoria`, `distancia`, `luz`) con 2 o 3 pistas que van de más vagas a más claras. Mateo lo usa desde `PedirPistaIntent`.
- Primer borrador de los **mockups**.

**Listo cuando:** el modelo compila, todas las frases resuelven bien en el profiler y `pistas.py` está en `main`.

## Iteración 2 — dom 22: mockups, plan de pruebas de voz y prueba con Mateo

**Mockups RF15**, en `docs/mockups/`, como imágenes con un `README.md`. Un panel para el **operador del escape room**:

| Elemento | Decisión que ayuda a tomar | Patrón de acceso |
|---|---|---|
| Partidas iniciadas y resueltas por día, y tasa de éxito | Si la caja es demasiado difícil o demasiado fácil | 2 |
| Tiempo medio y fallos por desafío | Qué desafío ajustar (bajar la dificultad o recalibrar) | 3 |
| Pistas pedidas por desafío | Qué desafío no se entiende | 4 |
| Historial de partidas de una caja | Revisar una sesión concreta | 1 |
| Alertas de cajas sin reportar | Qué caja revisar antes de la próxima sesión | 6 |

El `README.md` dice, para cada elemento, de qué consulta de DynamoDB sale: tabla, clave e índice. Eso es lo que pide la rúbrica.

**Plan de pruebas de voz**, en la sección de pruebas del informe: cada frase con su respuesta esperada. Incluye los casos de rechazo:
- "abre la caja" en modo juego;
- "inicia una partida" en modo manual;
- un parámetro fuera de rango;
- pedir una pista sin una partida en curso.

**Diagramas de secuencia de voz**, en Mermaid: una consulta ("¿cómo voy?") y un comando ("pon la dificultad en difícil"), desde el usuario hasta el objeto.

**Con Mateo:** probar la skill completa contra `caja-enigmas-02` y anotar lo que falle.

**Listo cuando:** los mockups están en el repo con sus consultas y la skill responde a todas las frases del plan.

## Iteración 3 — lun 23: pruebas de voz y costos

- Correr el plan de pruebas de voz con la caja real y guardar las capturas del simulador en `docs/evidencias/`.
- **Estimación de costos**: los mensajes por día que te pasa Adrián, las invocaciones de Lambda y las escrituras y lecturas de DynamoDB que te pasa Mateo. Compararlos con los límites de la capa gratuita.

## Iteración 4 — mar 24: cierre del informe y entrega

- Armar `INFORME_P4.md` a partir de los **registros de las iteraciones 0–3** de los cuatro planes y de las secciones que escribió cada uno. Cada campo del registro ya indica a qué sección va (tabla del [resumen](p4-00-resumen.md#registro-de-lo-aplicado-en-cada-iteración)). Revisar que el informe cumpla el §4.2.
- Escribir las tuyas: caso de uso ampliado y su justificación (junto con Santiago, la del HC-SR04), requerimientos, diseño de la skill, mockups, resultados, conclusiones (incluido el análisis crítico de la decisión Edge/Nube) y recomendaciones (extensiones hacia un uso real).
- Revisar con Adrián que no haya secretos, que los enlaces funcionen y que el repo sea accesible. **Entregar el enlace por el LMS antes de las 14:00.**

## Secciones del informe (esqueleto de `INFORME_P4.md`)

| Sección (§4.2) | Responsable |
|---|---|
| 1. Caso de uso y requerimientos | Lucía (con Santiago para el hardware) |
| 2. Diagrama de bloques y de circuito | Santiago |
| 2. Arquitectura, especificación del Shadow, estados de la lógica de control | Adrián |
| 2. Clases del firmware | Adrián y Santiago |
| 2. Clases de las Lambdas, modelo de DynamoDB, reglas de IoT y Lambdas | Mateo (Adrián, la vigilancia) |
| 2. Secuencias | Lucía (voz), Mateo (almacenamiento), Adrián (reglas automáticas) |
| 2. Diseño de la skill y mockups | Lucía |
| 3. Desarrollo e implementación | Cada uno, la parte de su código y de su configuración |
| 4. Pruebas y validaciones | Cada uno sus pruebas; Lucía las une |
| 5–8. Resultados, conclusiones, recomendaciones y anexos | Lucía, con datos de todos |

## Para la defensa: qué repasar de los demás

- `PartidaEnigmas` y el Shadow v2 de Adrián: por qué `solicitudPartida` es un contador, y qué pasa sin red.
- La Lambda de vigilancia: por qué va en la Nube y cada cuánto se ejecuta.
- La Lambda de almacenamiento y la tabla de Mateo: validaciones, claves, GSI y TTL.
- Los desafíos y el circuito de Santiago, y por qué se justifica el HC-SR04.

## Registro de lo aplicado

Llenar al cerrar cada iteración, con lo que **de verdad** quedó hecho. Si un campo no aplica, "—". Qué va en cada campo y a qué sección del informe alimenta: [p4-00-resumen.md](p4-00-resumen.md#registro-de-lo-aplicado-en-cada-iteración).

### Estado al 23/11/2026

La revisión `b7f3dd1` de `main` incorpora el backend de Mateo (PR #11) y los entregables de Lucía (PR #12). Sobre esa base se preparó la ampliación de frases de sensores y progreso.

| Iteración | Avance comprobado | Para cerrar |
|---|---|---|
| 0 · Skill e informe | «Caja de Enigmas» está configurada en desarrollo y en Spanish (US). El informe inicial ya está en `main`. | Confirmar la respuesta del endpoint en el simulador. |
| 1 · Modelo y pistas | El [modelo es-US](../../config/alexa/modelo-interaccion.es-US.json) se compiló con 14 intents propios y 114 frases. El [catálogo](../../lambda/pistas.py) aporta 3 pistas por desafío y ya está en `main`. | Confirmar los IDs de slots en el JSON del simulador. Conectar el catálogo a `obtener_pista_progresiva` y desplegar la Lambda de Mateo. |
| 2 · Reportes y voz | [Mockup RF15](../mockups/README.md) con consultas documentadas, [matriz de voz](../../INFORME_P4.md#83-matriz-de-frases-de-la-skill) y 17 casos de integración. | Adrián debe preparar el estado y el código de vinculación de `caja-enigmas-02`. Probar con Mateo desde Alexa y verificar los eventos de pista en DynamoDB. |
| 3 · Prueba física y costo | El firmware 0.4.0 integra la partida y los eventos. La estimación compara IoT Core, Lambda y DynamoDB con las referencias de AWS. | Confirmar la carga de P4 en `caja-enigmas-01`, ejecutar la prueba física y guardar evidencias. Completar el costo con el consumo observado y la elegibilidad de la cuenta. |
| 4 · Entrega | Las secciones de arquitectura, configuración, pruebas y costos de Adrián se integraron en el informe. | Añadir los resultados finales, revisar el PR del ajuste de voz y coordinar la entrega por el LMS. La consigna fija el límite del 24/11 a las 17:45, hora de La Paz. |

**RF4.** El contrato §7 y el modelo P4 incorporan `ConsultarSensoresIntent`. El resultado con Alexa y el Shadow de la caja de pruebas se registra en INT-16.

El 24/11 se añadieron diez frases de sensores y diez de progreso. Se guardó y compiló el modelo en Alexa y se verificó que el JSON exportado coincidiera con el del repositorio. Las 114 frases obtuvieron el intent esperado en la versión final. Se guardaron siete capturas en `docs/evidencias/alexa-p4/`. Siguen pendientes las evidencias del simulador con la Lambda y de operación de extremo a extremo.

#### Coordinación con Mateo

El 24/11 se verificó en Git la revisión `3dd2319` de la rama `feat/lambda-p4`. Mateo informó el despliegue de P4 en AWS, pruebas de RF4 con el Shadow y DynamoDB y 151 pruebas locales aprobadas. El catálogo de Lucía está listo para la integración. Queda confirmar la conexión en producción y preparar los tres estados de prueba para `caja-enigmas-02`.

- **Pistas:** Mateo implementó la secuencia 1 → 2 → 3 → 3 con reinicio por desafío, partida o sesión. Su proveedor recibe `(desafio, nivel)`. El archivo `lambda/pistas.py` contiene `PISTAS_POR_DESAFIO[desafio][nivel - 1]`. Falta conectarlo y revisar en DynamoDB un evento por pista entregada.
- **RF4:** Mateo implementó el manejador sin slots. Lucía añadió el intent y seis frases al modelo. El borrador compiló el 24/11 y las seis frases seleccionaron el intent esperado en Utterance Profiler. El manejador informa la medición disponible e identifica la ausente cuando recibe un reporte parcial.
- **Informe:** pedir a Mateo las evidencias disponibles de CloudWatch y DynamoDB, las exportaciones pendientes y los conteos con su ventana de medición para cerrar los resultados y los costos.
- **AWS y caja de pruebas:** Adrián informó que el rol desplegado ya permite publicar pistas. Queda exportar la política con ese permiso. También indicó que `caja-enigmas-02` carece del código de vinculación y ofreció preparar su estado de prueba. La caja física `caja-enigmas-01` sigue pendiente de confirmar con el firmware P4.

### Iteración 0 — vie 20
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en Alexa:**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 1 — sáb 21
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en Alexa:**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 2 — dom 22
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en Alexa:**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 3 — lun 23
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en Alexa:**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**
