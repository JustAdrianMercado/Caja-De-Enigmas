# Backend de la skill Caja de los Enigmas

Implementación Python de **Mateo Rojas** para AWS Lambda, región `us-east-1`.
Punto de entrada: `lambda_function.lambda_handler`; runtime objetivo Python 3.12.

- `lambda_function.py`: compone clientes, servicios y manejadores con `crear_skill`.
- `manejadores/`: solicitudes de Alexa; `servicios/`: DynamoDB y Shadow.
- `textos.py`: respuestas compartidas; `tests/`: suite local con AWS simulado.
- [requirements.txt](requirements.txt): rangos de dependencias de desarrollo.
- [requirements-despliegue.txt](requirements-despliegue.txt): versiones fijadas
  del paquete verificado.

Variables: `TABLA_VINCULACIONES`, `PREFIJO_THING` e `IOT_ENDPOINT`.
El backend usa el rol de ejecución; no se incluyen credenciales personales.

## RF4: consultar sensores de la caja vinculada

`ConsultarSensoresIntent`, sin slots, usa `ConsultarSensoresIntentHandler`,
registrado en `crear_skill`. Hereda de `IntentCajaHandler`: obtiene
`context.System.user.userId`, consulta `RepositorioVinculaciones.obtener`
y pasa únicamente el `thingName` vinculado a `ServicioShadow.leer_reported`.
No cambia las interfaces de los servicios ni añade dependencias.

Informa `distanciaCm` en centímetros y `luzPorcentaje` en porcentaje del
último reporte. Nunca sustituye mediciones con `desired`, distancia objetivo
o umbral de luz. Valida enteros según contratos.md §2: distancia 2–400 cm
y luz 0–100 %. Rechaza booleanos, decimales, cadenas y valores fuera de rango;
el cero de luz es válido. Una lectura ausente o inválida se explica y se
comunica la otra si está disponible; si faltan ambas, informa que no tiene
lecturas válidas. No calcula antigüedad ni acredita una medición en vivo.
Sin vinculación mantiene la ayuda existente; los errores de servicios se
propagan al manejador global, con respuesta segura y repregunta.

Evidencia local de TDD (Python 3.12.10):

- ROJO: al ejecutar el archivo nuevo antes de implementar, unittest falló
  al importarlo con `ModuleNotFoundError: manejadores.consultar_sensores`.
- VERDE: implementación y registro con dependencias existentes; se corrigió
  la serialización JSON de una prueba nueva de `can_handle`.
- REFACTOR: validación común de enteros y rangos con constantes descriptivas,
  salida temprana sin lecturas y composición sencilla de respuestas.
- `python -m unittest discover -s lambda/tests -p test_consultar_sensores.py -v`:
  **11 pruebas, OK**. Cubren ambas lecturas, cada lectura parcial, ausencia
  de ambas, vinculación e identidad ausentes, error Shadow, cero de luz,
  límites y tipos inválidos, separación de `desired` y objetivos, selección
  exclusiva del intent y registro mediante el dispatcher real del SDK.
  También comprueban ausencia de escrituras y conservación de la sesión.
- `python -m unittest discover -s lambda/tests -v`: **130 pruebas, OK**,
  incluidos cerradura, vinculación/desvinculación, partida, progreso, pistas,
  modo, dificultad, parámetros, registro y publicación de eventos simulados.

Pendiente de integración: Lucía debe añadir `ConsultarSensoresIntent` sin
slots al modelo de Alexa, con frases como «consulta los sensores»,
«qué distancia y luz mide mi caja», compilar y probar su reconocimiento.
El intent solicitado para RF4 todavía no figura en la tabla de contratos.md
§7: queda pendiente incorporarlo mediante el acuerdo del grupo. Esta tarea
solo actualiza documentación de Lambda; no cambia el contrato ni el modelo.

Después de revisar los cambios e integrar el modelo, preparar el ZIP con
las dependencias de despliegue existentes y cargarlo manualmente desde la
consola en `caja-enigmas-skill`, región `us-east-1`. Verificar configuración
y permisos de lectura existentes y probar en el simulador con una caja
vinculada y reportes completos y parciales. No se realizaron despliegues,
consultas a AWS ni pruebas físicas para RF4; las pruebas usan AWS simulado.

## Pistas progresivas: preparación local

El catálogo de Lucía (`lambda/pistas.py`) sigue ausente. No se crea un
catálogo sustituto ni se supone su estructura. La composición de producción
continúa sin proveedor: las pistas no se declaran integradas.

La auditoría identifica `session.sessionId` y `session.new` en la solicitud
de Alexa, `context.System.user.userId` como identidad y `reported.idPartida`,
`reported.desafioActual` y `reported.estadoPartida` como estado real de juego.
El repositorio DynamoDB solo conserva vinculaciones. No existe un adaptador
de persistencia del SDK configurado ni un contador previo de pistas.

El requisito de reinicio entre sesiones permite usar los atributos de sesión
existentes del SDK: `progresoPistas` conserva un único contexto de usuario,
caja, sesión, partida y desafío, con el último nivel entregado. Los atributos
se devuelven en `sessionAttributes` y Alexa los envía en solicitudes siguientes
de esa sesión; no recuperan progreso en una sesión nueva ni son persistencia
duradera. Tampoco se guarda estado en variables globales de Lambda.

`crear_skill` y `PedirPistaIntentHandler` conservan `obtener_pista(desafio)`
para consumidores actuales, con su comportamiento de pista única. Añaden
el argumento opcional `obtener_pista_progresiva(desafio, nivel)`, una función
inyectada que devuelve texto o ausencia de contenido. Esta firma es un punto
de adaptación local, **no una interfaz atribuida al catálogo de Lucía**.
Si se inyectan ambos proveedores, tiene prioridad el progresivo; no se
sustituye una pista progresiva ausente por contenido del proveedor anterior.

Con proveedor progresivo, los niveles son 1 (general), 2 (intermedio),
3 (explícito) y luego siempre 3. La correspondencia semántica de los textos
la debe aportar el catálogo real. Se exige partida `en_curso`, desafío
válido e `idPartida` entero no negativo según contratos.md §2. Un cambio
de contexto o una sesión nueva reinicia en 1. Una orden de inicio pendiente
no acredita una partida nueva: el reinicio se basa en el identificador
reportado por el dispositivo. Se conservan los atributos de otros intents.

El estado se prepara en el gestor de sesión antes de publicar; un fallo en
su lectura o asignación impide publicar y entregar la pista. Si publicar
falla, se restaura el estado anterior y responde el manejador global. Sin
contenido válido no se avanza ni publica. Cada entrega, incluso la repetición
de nivel 3, llama a `PublicadorEventos.publicar_pista` con el payload existente
`{"tipo":"pista","desafio":"...","origen":"voz"}`. No añade nivel,
sesión, usuario ni partida al evento, ni cambia Shadow o tablas.

Límites: los atributos de sesión no ofrecen transacciones con MQTT ni
confirmación de que Alexa habló. Un error posterior a publicar o una
retransmisión puede producir eventos repetidos; QoS 1 tampoco garantiza
exactamente una entrega. El éxito del SDK no prueba persistencia en el
histórico. La Lambda solo observa cambios incluidos en el último Shadow;
no puede detectar transiciones intermedias no observadas ni identificar una
partida cuyo identificador y contexto se reutilizaran. Resolver garantías
adicionales requeriría otro diseño y, posiblemente, cambios de contrato.

TDD local: primero las 12 pruebas iniciales fallaron por el argumento aún
inexistente `obtener_pista_progresiva`. Tras implementar, pasaron; se amplió
la cobertura a 16 pruebas y se revisaron validación, contexto y conservación
de atributos sin alterar pruebas existentes.

```powershell
python -m unittest discover -s lambda/tests -p test_pistas_progresivas.py -v
# 16 pruebas, OK
python -m unittest discover -s lambda/tests
# 146 pruebas, OK (incluye las 11 de RF4 y compatibilidad del proveedor anterior)
```

Las pruebas simulan proveedores y AWS, transportan los atributos entre
peticiones y cubren secuencia y repeticiones, reinicios por desafío, partida
y sesión, aislamiento de usuarios/cajas, ausencia de contenido, estado
corrupto y fallos de lectura, asignación, proveedor y publicación. También
verifican el payload real de `PublicadorEventos` con cliente simulado,
conservación del contador entre otros intents y otra instancia de la skill.

El arranque de producción conecta `obtener_pista_progresiva(desafio, nivel)`
con `PISTAS_POR_DESAFIO[desafio][nivel - 1]` de `pistas.py`. El catálogo de
Lucía contiene exactamente tres textos ordenados para memoria, distancia y
luz; se conservan sin cambios. La inyección opcional de `crear_skill` sigue
disponible para pruebas y consumidores anteriores.

`test_catalogo_pistas.py` ejecuta el arranque de producción con clientes AWS
simulados y servicios reales. Verifica los nueve textos completos, la
repetición de la tercera pista, un evento por entrega, los reinicios y las
consultas intercaladas de sensores, progreso y modo sin alterar el contador.
La suite completa de `lambda/tests` pasa con **155 pruebas**. También pasan
las suites de almacenamiento (**27**) y vigilancia (**20**).

Pendiente de validación externa: comprobar sesiones y frases en el simulador
de Alexa y la recepción de eventos en el histórico cuando el grupo despliegue.
No se modificaron el modelo de Alexa ni el firmware; no se realizaron
despliegues, commits, push ni PR.

## Iteración 2: backend local y dependencias pendientes

Se registran los ocho intents del contrato v2: `IniciarPartidaIntent`,
`PedirPistaIntent`, `ConsultarProgresoIntent`, `CambiarModoIntent`,
`ConsultarModoIntent`, `CambiarDificultadIntent`, `CambiarParametroIntent`
y `ConsultarParametrosIntent`. El dispatcher y la ayuda ya no ofrecen tapa.
La identidad sigue saliendo de `context.System.user.userId` y cada petición
resuelve la caja vinculada antes de acceder a Shadow o MQTT.

Los comandos anuncian una solicitud pendiente, sin afirmar que el objeto
la aplicó. Las consultas usan `reported`; un campo faltante no se completa
desde `desired`. La cerradura solo acepta comandos con modo `manual`
reportado; rechaza `juego` y modo desconocido sin escribir. Los slots propios
se aceptan por ID resuelto inequívoco, nunca por el texto del usuario.
El contrato fija comportamientos y un ejemplo de rechazo, no un catálogo
completo de respuestas literales; las demás respuestas locales explican
esos comportamientos sin atribuir frases nuevas al contrato.

**Bloqueos de integración comprobados en el checkout:** el modelo exportado
de `config/alexa/` sigue en P3, sin los ocho intents ni tipos propios, y
`lambda/pistas.py` no existe. Ambos archivos pertenecen a Lucía y no se
modifican. Los nombres, slots e IDs del backend siguen el contrato v2.
Las pruebas simulan sus solicitudes y no acreditan reconocimiento de voz.

`crear_skill` conserva sus argumentos originales y añade dependencias
opcionales `publicador_eventos` y `obtener_pista`. Esta última es un punto
de adaptación local: una función que recibe un desafío y devuelve el texto
disponible. **No es una interfaz supuesta de pistas.py.** La conexión al
archivo real, el nombre de su diccionario y la selección de sus pistas
quedan pendientes de acordar al recibirlo. Producción pasa `None`, sin
importaciones especulativas, contenido inventado ni errores de arranque.

`PedirPistaIntent` requiere partida `en_curso` y desafío válido. Si faltan
contenido o publicador, explica que las pistas no están disponibles y no
publica. Solo las pruebas inyectan contenido ficticio. Con contenido real,
el publicador envía `{"tipo":"pista","desafio":"memoria","origen":"voz"}`
al tema `caja-enigmas/<thingName>/eventos` (desafío según el reporte).
No añade `thingName`, `tipoMensaje`, `tsRegla` ni `idPartida` al payload.
La publicación correcta del SDK no confirma el almacenamiento posterior;
los errores pasan al manejador global seguro y no se entrega la pista
como si la publicación hubiera funcionado. El texto de pista se escapa
para SSML antes de responder.

Pasos siguientes, con intervención manual para las consolas:

1. Lucía exporta el modelo v2 con los nombres y slots de contratos.md §7,
   incluyendo IDs de `TipoModo`, `TipoDificultad` y `TipoParametro`, y quita
   `ConsultarTapaIntent` del modelo. Compilar y probar resolución de frases.
2. Recibir `pistas.py`, acordar su estructura y conectar un adaptador real
   en la composición. No crear el archivo de Lucía desde este backend.
3. Revisar manualmente el permiso `iot:Publish` del rol de la skill limitado
   a los temas `caja-enigmas/*/eventos`, según contrato §6.
4. Después de una integración autorizada, probar en el simulador contra
   `caja-enigmas-02`: órdenes en `desired`, estados de `reported`, rechazos,
   parámetros y recepción de pistas en el histórico. El checkout actual
   no se declara una skill P4 completa ni desplegada.

Desde la raíz, con dependencias instaladas:

```powershell
python -m unittest discover -s lambda/tests -v
```

Diseño y resultados: [INFORME_P4.md](../INFORME_P4.md).
Verificación del paquete local: [registro del 23/11/2026](caja-enigmas-skill-p4-20261123-verificacion.md).
