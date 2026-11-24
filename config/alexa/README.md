# Modelo de interacción de Alexa · Práctica 4

El [modelo de interacción](modelo-interaccion.es-US.json) usa **Spanish (US), es-US** y el nombre de invocación **caja de enigmas**. Conserva cinco intents de vinculación y cerradura de la Práctica 3, retira `ConsultarTapaIntent` y añade nueve intents de juego, configuración y consulta. La vinculación conserva los slots obligatorios `numeroCaja` y `codigo` con delegación automática.

## Compilación y prueba

1. Abrir el borrador de desarrollo de la skill en la [consola de Alexa](https://developer.amazon.com/alexa/console/ask) y seleccionar **Spanish (US)**.
2. En **Build → JSON Editor**, importar el JSON, guardar y ejecutar **Build Model**. Si el modelo cambia en la consola, exportar la versión que se probó.
3. En **Evaluate Model → Utterance Profiler**, ejecutar la matriz del [informe de la Práctica 4](../../INFORME_P4.md#41-reconocimiento-de-frases) y comprobar el intent, los slots y los IDs canónicos. Este paso funciona con el modelo compilado y no llama al endpoint. [Guía de Amazon](https://developer.amazon.com/en-US/docs/alexa/custom-skills/test-utterances-and-improve-your-interaction-model.html)
4. Coordinar con Mateo el endpoint de desarrollo y el Skill ID autorizado en el disparador de la Lambda. En **Test**, activar **Development**, abrir «caja de enigmas» y ejecutar las pruebas de integración con `caja-enigmas-02`. [Guía de Amazon](https://developer.amazon.com/en-US/docs/alexa/devconsole/test-your-skill.html)

## Integración con Lambda

- Los slots propios se resuelven por el ID del [contrato §7](../../docs/contratos.md#7-skill-de-alexa). Por ejemplo, «sencillo» resuelve a `facil` y «distancia» a `distanciaObjetivoCm`.
- [`lambda/pistas.py`](../../lambda/pistas.py) expone `PISTAS_POR_DESAFIO`, un diccionario de tuplas de tres textos de menor a mayor ayuda, con claves `memoria`, `distancia` y `luz`. Para el proveedor de Mateo `obtener_pista_progresiva(desafio, nivel)`, los niveles 1–3 corresponden a `PISTAS_POR_DESAFIO[desafio][nivel - 1]`. La Lambda administra el progreso y publica el evento.
- Estado al 24/11/2026. El borrador de desarrollo compiló con `ConsultarSensoresIntent` sin slots y las seis frases de RF4 se reconocieron en Utterance Profiler. El simulador y el registro de eventos de pistas en DynamoDB quedan pendientes. [Registro de Lucía](../../docs/planificacion/p4-04-lucia-skill-informe.md#estado-al-23112026)
