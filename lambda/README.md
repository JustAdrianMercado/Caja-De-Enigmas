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
