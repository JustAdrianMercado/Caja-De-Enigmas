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

Diseño y resultados: [INFORME.md](../INFORME.md).
Auditoría, paquete y receta de mantenimiento:
[backend-lambda.md](../docs/backend-lambda.md).
Preparación de la defensa: [GUIA_DEFENSA.md](../docs/GUIA_DEFENSA.md).
