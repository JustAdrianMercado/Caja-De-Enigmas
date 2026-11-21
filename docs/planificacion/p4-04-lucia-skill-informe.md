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
3. **`INFORME_P4.md`** con las secciones del §4.2 de la consigna y el responsable de cada una, según "Secciones del informe" más abajo. `INFORME.md` (P3) no se toca.

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
