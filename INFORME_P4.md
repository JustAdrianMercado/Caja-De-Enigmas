# Informe técnico — Práctica 4. Caja de los Enigmas

SIS-234 · Santiago Torres, Adrián Mercado, Mateo Rojas y Lucía Escobar

## 1. Caso de uso y requerimientos

### 1.1 Caso de uso ampliado

La Caja de los Enigmas es un módulo de escape room con tres desafíos físicos de memoria, distancia y luz. En la Práctica 3, Alexa servía para vincular la caja, consultar su estado y controlar la cerradura. En la Práctica 4, Alexa también inicia partidas, consulta su progreso y ofrece pistas. El ESP32 evalúa los desafíos localmente y libera la cerradura al completar la partida. El operador puede cambiar al modo manual y revisar el historial para identificar desafíos que necesitan ajustes.

### 1.2 Sensores y actuadores

El HC-SR04 mide la distancia de la mano sin contacto y permite verificar que se mantiene dentro del objetivo y su tolerancia. La LDR mide luz relativa para el desafío de iluminación. Los tres LEDs muestran la secuencia de memoria y los tres pulsadores de colores permiten repetirla. Un cuarto pulsador inicia la partida en modo juego. El servo SG90 libera o bloquea la cerradura. Pendiente con Santiago: incorporar los resultados de calibración del montaje.

### 1.3 Requerimientos funcionales y no funcionales

Los criterios de aceptación se basan en la [consigna](docs/consigna/Práctica_4.md) y el [contrato entre componentes](docs/contratos.md). Los resultados se registran en la sección 4.

| ID | Criterio de aceptación |
|---|---|
| RF1 | Medir distancia y luz, y detectar las pulsaciones de colores y de inicio. |
| RF2 | Publicar en `reported` las lecturas válidas y el estado de la cerradura, según la frecuencia acordada. |
| RF3 | Aplicar las órdenes válidas recibidas por `delta` y confirmar el estado efectivo en `reported`. |
| RF4 | Consultar por voz la distancia y la luz medidas de la caja vinculada mediante `ConsultarSensoresIntent`. Informar si una o ambas lecturas de `reported` no están disponibles. |
| RF5 | En modo manual, las órdenes de abrir o cerrar escriben en `desired.cerradura`. En modo juego se rechazan. |
| RF6 | Informar por voz el estado reportado de la cerradura. |
| RF7 | Vincular una caja existente con su código y mantener el vínculo al cambiar de sesión. |
| RF8 | Resolver la caja por el identificador de usuario de Alexa. Si el vínculo falta, explicar cómo crearlo. |
| RF9 | Validar existencia y código antes de vincular, y permitir reemplazar o eliminar la vinculación. |
| RF10 | Disparar el almacenamiento mediante las reglas de AWS IoT para lecturas y eventos. |
| RF11 | Validar los mensajes, descartar entradas inválidas y guardar lecturas con `thingName` y marca de tiempo. |
| RF12 | Guardar los eventos de cerradura con su origen. |
| RF13 | Ejecutar el juego y su control de cerradura en el ESP32, y detectar en la nube cajas que dejan de reportar. |
| RF14 | Consultar y cambiar modo, dificultad y calibración por voz, con confirmación en `reported`. |
| RF15 | Diseñar el panel de reportes y documentar la consulta DynamoDB de cada indicador. |

| Criterio no funcional | Verificación prevista |
|---|---|
| Estabilidad | Al menos 10 minutos de partidas seguidas con cero reinicios. |
| Exactitud y calibración | Diez mediciones del HC-SR04 a 10, 20 y 30 cm contra una regla. Diez lecturas de LDR por condición de luz. Registrar errores, dispersión y umbral elegido. |
| Respuesta del actuador | Medir por separado la latencia de órdenes de voz y del desbloqueo automático del ESP32. Acordar el máximo para la prueba P4 antes de ejecutarla. |
| Reporte | Comprobar publicación por cambio de estado en un máximo de 1 s y reporte completo cada 60 s. |
| Operación sin conexión | Completar una partida al cortar el WiFi. Al volver la red, verificar el envío en orden de los eventos retenidos y la aplicación de `desired`. El contrato conserva hasta 20 eventos y solo reporta la lectura actual al reconectar. |
| Integridad del almacenamiento | Con una ventana declarada, comparar publicaciones válidas únicas con ítems guardados. Objetivo de la rúbrica ≥ 99 %. Enviar también entradas inválidas y comprobar su descarte. |
| Latencia de almacenamiento | Medir desde la publicación del objeto hasta el registro. Usar una marca del objeto sincronizada para medir el recorrido completo. Referencia ≤ 5 s. |
| Vigilancia | La alerta por falta de reportes aparece en un máximo esperado de 20 minutos en condiciones normales, según el umbral de 15 minutos y la regla de 5 minutos. |
| Costo | Estimar el consumo mensual de IoT Core, Lambda y DynamoDB a partir de mediciones del prototipo y contrastarlo con la capa gratuita aplicable. |

## 2. Análisis y diseño

| Sección | Responsable |
|---|---|
| Diagrama de bloques y circuito | Santiago |
| Arquitectura y Shadow actualizado | Adrián |
| Diagrama de clases del ESP32 | Adrián y Santiago |
| Diagramas de clases de las Lambdas | Mateo |
| Secuencias de voz, almacenamiento y reglas automáticas | Lucía, Mateo y Adrián, respectivamente |
| Estados, modos, transiciones y justificación Edge/Nube | Adrián |
| Modelo DynamoDB y patrones de acceso | Mateo |
| Reglas IoT y Lambdas | Mateo y Adrián |
| Modelo de interacción de Alexa | Lucía |
| Mockups de reportes RF15 y consultas de origen | Lucía |

### 2.1 Diseño de la skill

La skill usa `es-US` y responde a «caja de enigmas». Conserva la vinculación de caja y el control de la cerradura en modo manual. Los [intents, slots y frases](config/alexa/modelo-interaccion.es-US.json) se definen en el modelo de interacción. La [matriz de voz](#41-reconocimiento-de-frases) documenta sus efectos esperados. Las consultas se basan en `reported` y las órdenes pasan por `desired`.

### 2.2 Secuencias de voz

**Consulta «¿cómo voy?»**

```mermaid
sequenceDiagram
    participant Objeto as ESP32
    participant Shadow as Device Shadow
    actor Jugador
    participant Alexa
    participant Skill as Lambda de la skill
    participant Vinculos as DynamoDB
    Objeto->>Shadow: Reporta estadoPartida y desafioActual
    Jugador->>Alexa: ¿Cómo voy?
    Alexa->>Skill: ConsultarProgresoIntent y userId
    Skill->>Vinculos: Busca la caja vinculada
    Vinculos-->>Skill: thingName
    Skill->>Shadow: Lee reported de la caja
    Shadow-->>Skill: Último estado y desafío
    Skill-->>Alexa: Respuesta en español
    Alexa-->>Jugador: Informa el progreso reportado
```

**Comando «pon la dificultad en difícil»**

```mermaid
sequenceDiagram
    actor Operador
    participant Alexa
    participant Skill as Lambda de la skill
    participant Vinculos as DynamoDB
    participant Shadow as Device Shadow
    participant Objeto as ESP32
    Operador->>Alexa: Pon la dificultad en difícil
    Alexa->>Skill: CambiarDificultadIntent y slot dificil
    Skill->>Vinculos: Busca la caja por userId
    Vinculos-->>Skill: thingName
    Skill->>Shadow: Escribe desired.dificultad = dificil
    Shadow-->>Skill: Acepta la actualización
    Skill-->>Alexa: Solicitud enviada, confirmación pendiente
    Alexa-->>Operador: Informa el envío
    Shadow-->>Objeto: Entrega delta cuando difiere de reported
    Objeto->>Objeto: Adopta parámetros para el siguiente inicio de desafío
    Objeto->>Shadow: Reporta dificultad, longitud y tolerancia
```

### 2.3 Reportes para el operador

El [mockup del panel RF15](docs/mockups/README.md) reúne partidas iniciadas y resueltas, tasa de éxito, tiempo medio, fallos, pistas, historial y alertas. La guía identifica las claves y los filtros que sustentan cada dato.

### 2.4 Arquitectura, Shadow y reglas automáticas

El ESP32 decide el progreso y el desbloqueo a partir de los sensores locales. Alexa consulta el estado y envía órdenes mediante el Device Shadow. AWS IoT enruta los reportes y eventos a la Lambda de almacenamiento. La Lambda de vigilancia busca lecturas recientes en DynamoDB y registra una alerta cuando falta el reporte de una caja. El panel consulta esas alertas mediante el índice `alertas-por-fecha`.

El [aporte de Adrián](docs/informe-p4-adrian.md) incluye los diagramas de arquitectura, clases, estados y secuencias automáticas. El [contrato actualizado](docs/contratos.md) especifica el Shadow y las reglas de publicación e histórico.

## 3. Desarrollo e implementación

El [firmware 0.4.0](firmware/src/main.cpp) integra los tres desafíos, el servo, el inicio por voz y el pulsador. La publicación de eventos sale de una cola de hasta 20 entradas, una por vuelta de `loop()`. Si la cola se llena, se descarta el más antiguo. Si se rechaza el mismo evento tres veces con la caja conectada, se descarta y se contabiliza.

El backend de Mateo registra los ocho intents nuevos. El [almacenamiento](lambda-almacenamiento/README.md) valida lecturas y eventos antes de guardarlos. La [vigilancia](lambda-vigilancia/README.md) consulta la tabla principal y distingue lecturas de eventos al comprobar si la caja sigue reportando. El código de los tres componentes está en la base local. La configuración y las evidencias de despliegue se incorporarán a partir de los registros de cada responsable. El montaje y la calibración finales siguen a cargo de Santiago.

La [guía de Alexa](config/alexa/README.md) documenta la importación y compilación del modelo, la evaluación de las frases y la coordinación del endpoint. [El módulo de pistas](lambda/pistas.py) contiene tres textos por desafío ordenados de menor a mayor ayuda. La selección y su publicación como evento se integran con Mateo en la Lambda.

## 4. Pruebas y validaciones

### 4.1 Reconocimiento de frases

Compilar la versión `es-US` y probar cada frase con Utterance Profiler. Registrar el intent, el valor reconocido y el ID de entidad para los slots propios. En las pruebas de integración, usar el código real de la caja en lugar del ejemplo `0481` y registrar la respuesta de Alexa y el efecto observado. Guardar la evidencia en `docs/evidencias/` sin mostrar códigos ni identificadores de cuentas.

El 23/11/2026 se importó el modelo en el borrador de desarrollo de «Caja de Enigmas». En la primera compilación, 82 de 88 frases resolvieron el intent esperado. Se sustituyeron seis consultas que devolvían `AMAZON.FallbackIntent`. La segunda compilación fue exitosa y **88 de 88 frases de la matriz revisada** resolvieron el intent esperado. Los seis diálogos con datos de vinculación incompletos pidieron los slots faltantes y conservaron el cero inicial del código de ejemplo.

La tabla registra las entradas de texto y los valores de slot que mostró Utterance Profiler. Quedan por comprobar los IDs canónicos en el JSON de solicitud del simulador, las respuestas de Lambda y los efectos sobre el Shadow y el histórico.

El 24/11/2026 se compiló el modelo con RF4. Las seis frases nuevas seleccionaron `ConsultarSensoresIntent`. Se comprobó también que cuatro frases de control conservaban sus intents de progreso, parámetros, cerradura y cambio de distancia.

El prefijo de cada caso indica el intent y el resultado funcional esperado. Para control de cerradura, usar modo manual. Para inicio y pistas, usar modo juego y restaurar el estado de prueba entre casos según corresponda.

| Prefijo | Intent esperado | Respuesta o efecto esperado en el simulador |
|---|---|---|
| VIN | `VincularCajaIntent` | Valida el código y confirma que la caja quedó vinculada. |
| DES | `DesvincularCajaIntent` | Confirma la desvinculación. Volver a vincular antes de la siguiente frase. |
| CER | `ConsultarCerraduraIntent` | Informa la cerradura reportada y avisa si hay una orden pendiente. |
| ABR | `AbrirCerraduraIntent` | En modo manual, solicita abrir la cerradura. Verificar `desired.cerradura = abierta`. |
| CIE | `CerrarCerraduraIntent` | En modo manual, solicita cerrar la cerradura. Verificar `desired.cerradura = cerrada`. |
| INI | `IniciarPartidaIntent` | En modo juego, solicita iniciar la partida. Verificar que aumenta `desired.solicitudPartida`. |
| PIS | `PedirPistaIntent` | Con una partida en curso, da una pista del desafío actual y publica el evento `pista`. |
| PRO | `ConsultarProgresoIntent` | Informa el estado de la partida y el desafío actual si está en curso, según `reported`. |
| MOD | `CambiarModoIntent` | Confirma que envió la solicitud de modo. Verificar el ID indicado en `desired.modo`. |
| CMO | `ConsultarModoIntent` | Informa el modo de `reported.modo`. |
| DIF | `CambiarDificultadIntent` | Confirma que envió la solicitud. Verificar el ID indicado en `desired.dificultad`. |
| PAR | `CambiarParametroIntent` | Confirma que envió la solicitud. Verificar el valor del parámetro indicado en `desired`. |
| CPA | `ConsultarParametrosIntent` | Informa dificultad, longitud de secuencia, tolerancia, distancia objetivo y umbral de luz según `reported`. |
| SEN | `ConsultarSensoresIntent` | Informa la distancia medida en centímetros y la luz medida en porcentaje. Si falta una lectura, informa la disponible. |

| Caso | Frase | Slots esperados | Observado / evidencia |
|---|---|---|---|
| VIN-1 | vincula mi caja dos con el código cero cuatro ocho uno | `numeroCaja = 2`, `codigo = 0481` | `VincularCajaIntent`. Se mostraron `codigo:0481 numeroCaja:2`. |
| VIN-2 | registra la caja dos con el código cero cuatro ocho uno | `numeroCaja = 2`, `codigo = 0481` | `VincularCajaIntent`. Se mostraron `codigo:0481 numeroCaja:2`. |
| VIN-3 | vincula mi caja | solicita los datos faltantes | `VincularCajaIntent`. Solicitó los datos faltantes y terminó con caja 2 y código de ejemplo de cuatro dígitos. |
| VIN-4 | registra mi caja | solicita los datos faltantes | `VincularCajaIntent`. Solicitó los datos faltantes y terminó con caja 2 y código de ejemplo de cuatro dígitos. |
| VIN-5 | vincula la caja dos | `numeroCaja = 2`, solicita los datos faltantes | `VincularCajaIntent`. Solicitó los datos faltantes y terminó con caja 2 y código de ejemplo de cuatro dígitos. |
| VIN-6 | registra la caja dos | `numeroCaja = 2`, solicita los datos faltantes | `VincularCajaIntent`. Solicitó los datos faltantes y terminó con caja 2 y código de ejemplo de cuatro dígitos. |
| VIN-7 | cambia mi caja | solicita los datos faltantes | `VincularCajaIntent`. Solicitó los datos faltantes y terminó con caja 2 y código de ejemplo de cuatro dígitos. |
| VIN-8 | asocia mi caja | solicita los datos faltantes | `VincularCajaIntent`. Solicitó los datos faltantes y terminó con caja 2 y código de ejemplo de cuatro dígitos. |
| DES-1 | desvincula mi caja | — | `DesvincularCajaIntent`. |
| DES-2 | olvida mi caja | — | `DesvincularCajaIntent`. |
| DES-3 | elimina mi vinculación | — | `DesvincularCajaIntent`. |
| DES-4 | quita mi caja | — | `DesvincularCajaIntent`. |
| DES-5 | desasocia mi caja | — | `DesvincularCajaIntent`. |
| DES-6 | borra la vinculación | — | `DesvincularCajaIntent`. |
| DES-7 | quiero desvincular mi caja | — | `DesvincularCajaIntent`. |
| DES-8 | deja de usar mi caja | — | `DesvincularCajaIntent`. |
| CER-1 | cómo está la cerradura | — | `ConsultarCerraduraIntent`. |
| CER-2 | está trabada la caja | — | `ConsultarCerraduraIntent`. |
| CER-3 | está destrabada la caja | — | `ConsultarCerraduraIntent`. |
| CER-4 | la cerradura está abierta | — | `ConsultarCerraduraIntent`. |
| CER-5 | la cerradura está cerrada | — | `ConsultarCerraduraIntent`. |
| CER-6 | consulta la cerradura | — | `ConsultarCerraduraIntent`. |
| CER-7 | dime el estado de la cerradura | — | `ConsultarCerraduraIntent`. |
| CER-8 | está cerrada la cerradura | — | `ConsultarCerraduraIntent`. |
| ABR-1 | abre la cerradura | — | `AbrirCerraduraIntent`. |
| ABR-2 | destraba la caja | — | `AbrirCerraduraIntent`. |
| ABR-3 | abre mi caja | — | `AbrirCerraduraIntent`. |
| ABR-4 | desbloquea la cerradura | — | `AbrirCerraduraIntent`. |
| ABR-5 | quiero abrir la caja | — | `AbrirCerraduraIntent`. |
| ABR-6 | destraba mi caja | — | `AbrirCerraduraIntent`. |
| ABR-7 | abre la caja | — | `AbrirCerraduraIntent`. |
| ABR-8 | desbloquea mi caja | — | `AbrirCerraduraIntent`. |
| CIE-1 | cierra la cerradura | — | `CerrarCerraduraIntent`. |
| CIE-2 | traba la caja | — | `CerrarCerraduraIntent`. |
| CIE-3 | cierra mi caja | — | `CerrarCerraduraIntent`. |
| CIE-4 | bloquea la cerradura | — | `CerrarCerraduraIntent`. |
| CIE-5 | quiero cerrar la caja | — | `CerrarCerraduraIntent`. |
| CIE-6 | traba mi caja | — | `CerrarCerraduraIntent`. |
| CIE-7 | cierra la caja | — | `CerrarCerraduraIntent`. |
| CIE-8 | bloquea mi caja | — | `CerrarCerraduraIntent`. |
| INI-1 | empecemos a jugar | — | `IniciarPartidaIntent`. |
| INI-2 | inicia una partida | — | `IniciarPartidaIntent`. |
| INI-3 | comienza la partida | — | `IniciarPartidaIntent`. |
| INI-4 | quiero jugar | — | `IniciarPartidaIntent`. |
| INI-5 | inicia el juego | — | `IniciarPartidaIntent`. |
| INI-6 | arranca una partida | — | `IniciarPartidaIntent`. |
| PIS-1 | quiero una pista | — | `PedirPistaIntent`. |
| PIS-2 | dame una pista | — | `PedirPistaIntent`. |
| PIS-3 | necesito una pista | — | `PedirPistaIntent`. |
| PIS-4 | dame una sugerencia | — | `PedirPistaIntent`. |
| PIS-5 | pide una pista | — | `PedirPistaIntent`. |
| PIS-6 | dame una pista para este desafío | — | `PedirPistaIntent`. |
| PRO-1 | cómo voy | — | `ConsultarProgresoIntent`. |
| PRO-2 | consulta mi desafío actual | — | `ConsultarProgresoIntent`. |
| PRO-3 | dime mi progreso | — | `ConsultarProgresoIntent`. |
| PRO-4 | cuál es mi progreso | — | `ConsultarProgresoIntent`. |
| PRO-5 | cuál es mi desafío actual | — | `ConsultarProgresoIntent`. |
| PRO-6 | consulta mi progreso | — | `ConsultarProgresoIntent`. |
| MOD-1 | pon la caja en modo manual | `modo → manual` | `CambiarModoIntent`. Se mostraron `modo:manual`. |
| MOD-2 | activa el modo juego | `modo → juego` | `CambiarModoIntent`. Se mostraron `modo:juego`. |
| MOD-3 | cambia el modo a operador | `modo → manual` | `CambiarModoIntent`. Se mostraron `modo:operador`. |
| MOD-4 | usa el modo automático | `modo → juego` | `CambiarModoIntent`. Se mostraron `modo:automatico`. |
| MOD-5 | cambia al modo manual | `modo → manual` | `CambiarModoIntent`. Se mostraron `modo:manual`. |
| MOD-6 | configura el modo juego | `modo → juego` | `CambiarModoIntent`. Se mostraron `modo:juego`. |
| CMO-1 | en qué modo está la caja | — | `ConsultarModoIntent`. |
| CMO-2 | cuál es el modo actual | — | `ConsultarModoIntent`. |
| CMO-3 | consulta el modo | — | `ConsultarModoIntent`. |
| CMO-4 | dime el modo de la caja | — | `ConsultarModoIntent`. |
| CMO-5 | dime el modo actual | — | `ConsultarModoIntent`. |
| CMO-6 | consulta el modo de la caja | — | `ConsultarModoIntent`. |
| DIF-1 | pon la dificultad en difícil | `dificultad → dificil` | `CambiarDificultadIntent`. Se mostraron `dificultad:dificil`. |
| DIF-2 | sube la dificultad a complicado | `dificultad → dificil` | `CambiarDificultadIntent`. Se mostraron `dificultad:complicado`. |
| DIF-3 | baja la dificultad a sencillo | `dificultad → facil` | `CambiarDificultadIntent`. Se mostraron `dificultad:sencillo`. |
| DIF-4 | ajusta la dificultad a medio | `dificultad → normal` | `CambiarDificultadIntent`. Se mostraron `dificultad:medio`. |
| DIF-5 | configura la dificultad en fácil | `dificultad → facil` | `CambiarDificultadIntent`. Se mostraron `dificultad:facil`. |
| DIF-6 | cambia la dificultad a normal | `dificultad → normal` | `CambiarDificultadIntent`. Se mostraron `dificultad:normal`. |
| PAR-1 | cambia la distancia objetivo a veinte | `parametro → distanciaObjetivoCm`, `valor = 20` | `CambiarParametroIntent`. Se mostraron `parametro:distancia objetivo valor:20`. |
| PAR-2 | ajusta el umbral de luz a setenta | `parametro → umbralLuzPorcentaje`, `valor = 70` | `CambiarParametroIntent`. Se mostraron `parametro:umbral de luz valor:70`. |
| PAR-3 | configura distancia en veinticinco | `parametro → distanciaObjetivoCm`, `valor = 25` | `CambiarParametroIntent`. Se mostraron `parametro:distancia valor:25`. |
| PAR-4 | pon luz en ochenta | `parametro → umbralLuzPorcentaje`, `valor = 80` | `CambiarParametroIntent`. Se mostraron `parametro:luz valor:80`. |
| PAR-5 | establece distancia objetivo en treinta | `parametro → distanciaObjetivoCm`, `valor = 30` | `CambiarParametroIntent`. Se mostraron `parametro:distancia objetivo valor:30`. |
| PAR-6 | fija umbral de luz en sesenta | `parametro → umbralLuzPorcentaje`, `valor = 60` | `CambiarParametroIntent`. Se mostraron `parametro:umbral de luz valor:60`. |
| CPA-1 | cuáles son los parámetros del juego | — | `ConsultarParametrosIntent`. |
| CPA-2 | consulta los parámetros | — | `ConsultarParametrosIntent`. |
| CPA-3 | dime la configuración del juego | — | `ConsultarParametrosIntent`. |
| CPA-4 | dime los parámetros activos del juego | — | `ConsultarParametrosIntent`. |
| CPA-5 | lee los ajustes del juego | — | `ConsultarParametrosIntent`. |
| CPA-6 | consulta la configuración del juego | — | `ConsultarParametrosIntent`. |
| SEN-1 | consulta las lecturas de los sensores | — | `ConsultarSensoresIntent`. |
| SEN-2 | dime la distancia y la luz | — | `ConsultarSensoresIntent`. |
| SEN-3 | dime la distancia medida | — | `ConsultarSensoresIntent`. |
| SEN-4 | dime cuánta luz mide la caja | — | `ConsultarSensoresIntent`. |
| SEN-5 | consulta las mediciones de la caja | — | `ConsultarSensoresIntent`. |
| SEN-6 | dime las lecturas de distancia y luz | — | `ConsultarSensoresIntent`. |

### 4.2 Integración de voz y rechazos

Con Mateo, vincular `caja-enigmas-02` a la cuenta de prueba y habilitar **Development**. Comparar la respuesta, `desired`, `reported` y los eventos, según el caso. En la caja física, comprobar también que el servo y el juego ejecuten las órdenes.

#### Orden de ejecución con Mateo

1. Confirmar el endpoint de desarrollo, la revisión de Lambda desplegada y el disparador de Alexa. Abrir «caja de enigmas» en el simulador de texto y registrar la respuesta inicial.
2. Usar una cuenta de prueba vinculada a `caja-enigmas-02`. Con el estado de prueba preparado por Mateo, empezar por consultar modo, cerradura, progreso y parámetros. Revisar en **Skill I/O** los IDs canónicos de slots cuando se prueben los comandos.
3. Ejecutar los rechazos por modo y rango. Comparar `desired` antes y después. Después, comprobar los cambios permitidos de modo, dificultad y calibración en la caja de pruebas.
4. Probar la solicitud de pista con un desafío en curso. Registrar la respuesta y buscar en `CajaEnigmasHistorico` el evento `pista` de esa caja con el desafío y `origen = voz`. Repetir sin partida activa y comprobar que la petición no genera un evento.
5. Probar `ConsultarSensoresIntent` con un reporte completo, con una lectura ausente y sin lecturas. Dejar la validación física de `caja-enigmas-01` para cuando un integrante pueda observar el servo.

#### Estados de prueba para caja-enigmas-02

Mateo preparará estos estados en el Shadow de la caja de pruebas. Para cada caso, registrar `reported` y los campos relevantes de `desired` antes y después. Los valores de esta tabla son entradas de prueba propuestas.

| Estado | Campos de `reported` | Qué se comprueba |
|---|---|---|
| Juego en curso | `modo = juego`, `estadoPartida = en_curso`, `desafioActual = memoria`, `idPartida = 1` | «Cómo voy» informa memoria, «quiero una pista» entrega una pista y «abre la caja» conserva `desired.cerradura`. Repetir con distancia y luz para verificar el catálogo. |
| Juego en espera | `modo = juego`, `estadoPartida = esperando`, `desafioActual = ninguno`, `solicitudPartida = 0` | «Quiero una pista» se rechaza y no genera evento. «Inicia una partida» solicita `desired.solicitudPartida = 1`. |
| Modo manual | `modo = manual`, `estadoPartida = esperando`, `desafioActual = ninguno`, `cerradura = cerrada`, `solicitudPartida = 0` | «Inicia una partida» se rechaza y conserva el contador. «Abre la caja» solicita `desired.cerradura = abierta`. |

Para RF4, usar `distanciaCm = 23` y `luzPorcentaje = 68` para verificar la respuesta con ambas lecturas y quitar un campo para repetir la prueba con un reporte parcial. Cuando falten ambos campos, Alexa debe comunicar que las lecturas no están disponibles.

| Caso | Frase o procedimiento | Resultado esperado | Observado / evidencia |
|---|---|---|---|
| INT-01 | «abre caja de enigmas» | La skill abre una sesión en español y presenta las operaciones disponibles. | Pendiente |
| INT-02 | «abre la caja» en modo juego | Explica que la orden corresponde al modo manual. `desired.cerradura` conserva su valor. | Pendiente |
| INT-03 | «inicia una partida» en modo manual | Pide pasar a modo juego. El contador y el estado de la partida conservan su valor. | Pendiente |
| INT-04 | «cambia la distancia objetivo a cincuenta» | Explica el rango de 10 a 40 cm. El parámetro conserva su valor. | Pendiente |
| INT-05 | «cambia el umbral de luz a cinco» | Explica el rango de 10 a 95 %. El parámetro conserva su valor. | Pendiente |
| INT-06 | «quiero una pista» con la caja en espera o la partida resuelta | Explica que no hay partida en curso. No se publica un evento `pista`. | Pendiente |
| INT-07 | Pedir una pista en cada desafío | La pista corresponde al desafío y se registra un evento `pista` con origen `voz`. | Pendiente |
| INT-08 | «pon la caja en modo juego» desde modo manual | `desired.modo = juego` y se elimina cualquier orden pendiente en `desired.cerradura`. | Pendiente |
| INT-09 | «cómo voy» con cuenta sin vínculo | Alexa explica cómo vincular una caja. | Pendiente |
| INT-10 | Vincular con código incorrecto o número inexistente | Rechaza el vínculo y conserva cualquier vínculo anterior. | Pendiente |
| INT-11 | Vincular otra caja válida, cerrar sesión y volver a consultar | La nueva vinculación persiste y la consulta usa el Shadow de la nueva caja. | Pendiente |
| INT-12 | Dos cuentas con cajas distintas | Cada cuenta consulta y controla su propia caja. | Pendiente |
| INT-13 | «cuáles son los parámetros del juego» con reporte parcial | Alexa informa solo los campos disponibles y advierte que el reporte está incompleto. | Pendiente |
| INT-14 | Probar «ayuda», «cancela», «para», «ve al inicio» y una petición fuera de tema | Se resuelven Help, Cancel, Stop, NavigateHome y Fallback. Registrar cada variante por separado. | Pendiente |
| INT-15 | «cambia la distancia objetivo a veinte centímetros» | Verificar que la variante con unidad resuelva `CambiarParametroIntent` con `distanciaObjetivoCm` y valor `20`. | En ambas compilaciones, Utterance Profiler seleccionó `CambiarParametroIntent` y mostró `valor:?`. La frase sin unidad (PAR-1) devolvió `valor:20`. |
| INT-16 | «dime la distancia y la luz» con ambas mediciones, una sola y ninguna | Informa los valores medidos en centímetros y porcentaje e indica las lecturas ausentes. El Shadow conserva sus valores. | Pendiente en el simulador |
| INT-17 | Pedir cuatro pistas seguidas y luego cambiar de desafío, de partida o de sesión | Secuencia 1 → 2 → 3 → 3. Cada contexto nuevo reinicia en 1. Se registra un evento `pista` por respuesta entregada. | Pendiente de conectar el catálogo y comprobar DynamoDB |

### 4.3 Pruebas del dispositivo y del almacenamiento

Adrián registra 89 pruebas nativas de firmware y 20 pruebas de vigilancia aprobadas, además de una compilación para la placa. Faltan las mediciones de hardware, la pérdida y recuperación de conexión con la caja física y la verificación de alertas en AWS. [Registro de Adrián](docs/planificacion/p4-02-adrian-firmware-aws-iot.md#iteración-2--dom-22-terminada-el-lun-23)

En la prueba inicial de recepción, Mateo registró `persistido = false`. En una actualización posterior, Mateo confirmó que guardó eventos y lecturas de `caja-enigmas-02` en DynamoDB y que dispone de evidencias de CloudWatch y DynamoDB. También reportó **119 pruebas locales aprobadas** para el backend P4. Falta incorporar esas evidencias y exportaciones al repositorio, además de las mediciones de integridad y latencia. [Registro inicial de Mateo](docs/planificacion/p4-03-mateo-lambda.md#iteración-0--vie-20)

Mateo informó que desplegó la Lambda P4 y probó RF4 con el Shadow y DynamoDB. Su rama `feat/lambda-p4` documenta **151 pruebas exitosas** en `3dd2319`. El código de la rama incluye el proveedor de pistas progresivas como dependencia opcional. Quedan pendientes conectar el catálogo de Lucía al proveedor de producción y verificar en el simulador los casos de RF4 y la persistencia de eventos de pistas.

## 5. Resultados

El modelo de desarrollo incluye 14 intents propios y 94 frases. Se verificaron las 88 frases en la compilación del 23/11 y las seis de RF4, más cuatro frases de control, en la del 24/11. Se prepararon nueve pistas y el mockup RF15 con datos ilustrativos. Quedan pendientes la integración de las pistas y las pruebas funcionales y físicas. [Estado de Lucía](docs/planificacion/p4-04-lucia-skill-informe.md#estado-al-23112026)

### 5.1 Estimación preliminar de uso

Con el escenario de Adrián de una caja, 20 partidas diarias sin fallos y un mes de 30 días, se estiman 1 440 reportes periódicos diarios y unos 11 mensajes adicionales por partida. Eso da aproximadamente **1 660 publicaciones diarias y 49 800 al mes** de la caja. Cada fallo de memoria aporta un evento adicional y cada pista solicitada agrega un evento de la skill.

La regla de vigilancia cada 5 minutos implica **288 ejecuciones diarias y 8 640 mensuales**. Cada ejecución inicia una consulta por caja y puede leer páginas adicionales. Solo se escribe al crear una alerta nueva. Para cerrar el cálculo faltan los conteos de mensajes medidos, las invocaciones de la skill y del almacenamiento, las lecturas y escrituras de DynamoDB, y el contraste con la capa gratuita aplicable. [Supuestos de Adrián](docs/informe-p4-adrian.md)

## 6. Conclusiones

El grupo evaluará el cumplimiento de los requerimientos y los efectos de ejecutar la lógica de partida en el dispositivo y la vigilancia en la nube.

## 7. Recomendaciones

Lucía y el grupo propondrán mejoras a partir de los fallos y limitaciones observados durante las pruebas.

## 8. Anexos

Incluir diagramas, mockups, configuraciones exportadas, fotografías, evidencias de prueba y muestras de los registros almacenados.
