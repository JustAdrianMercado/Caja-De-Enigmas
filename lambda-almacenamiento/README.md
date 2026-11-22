# Almacenamiento: iteración 1 local

Responsable: Mateo. Contrato: [v2](../docs/contratos.md), §§2, 3.2, 5.2 y 6.
Runtime: Python 3.12; handler: `lambda_function.lambda_handler`.

## Archivos y flujo

- `validacion.py`: reconoce las dos salidas de IoT Rules, valida tipos,
  rangos, fechas y campos obligatorios específicos de cada evento.
- `enriquecimiento.py`: construye el ítem con los campos permitidos.
- `servicios/repositorio_historico.py`: recibe un objeto con interfaz
  `DynamoDB.Table.put_item(Item=...)`; no usa `DescribeTable`.
- `lambda_function.py`: compone y procesa; `procesar_mensaje` permite inyectar
  repositorio y reloj sin crear clientes AWS.
- `tests/test_recepcion.py`: usa tabla, repositorio y reloj falsos.
- `preparar_paquete.py`: crea el ZIP con una lista explícita de fuentes.

El módulo se importa sin boto3 ni búsqueda de credenciales. Solo al procesar
un mensaje válido sin repositorio inyectado se inicializa boto3 en us-east-1.
No invocar el handler de producción localmente con un mensaje válido salvo
que se sustituya la fábrica del repositorio, como hacen las pruebas.

## Validación

`thingName` debe tener formato `caja-enigmas-NN`. `tsRegla` es un entero epoch
no negativo en ms, representable por datetime. Los booleanos no son enteros
válidos. Son obligatorios los metadatos añadidos por la regla; no se extrae
la identidad del dispositivo de otro campo del payload.

Lecturas: `tipo = lectura`, sin `tipoMensaje`, con distancia (2–400 cm),
luz (0–100 %) o ambas. Si un sensor está presente con valor inválido se
rechaza el mensaje completo. Cada lectura válida provoca un PutItem aunque
sus valores no cambien.

Eventos: `tipoMensaje = evento`, manteniendo el nombre del evento en `tipo`.

| Evento | Campos obligatorios |
|---|---|
| partida_iniciada | idPartida, origen (edge o voz), dificultad |
| desafio_superado | idPartida, desafio, duracionMs, intentos |
| desafio_fallado | idPartida, desafio |
| partida_resuelta | idPartida, duracionMs |
| cerradura | cerradura, origen |
| pista | desafio, origen (voz) |

`idPartida >= 1`, `duracionMs` entre 0 y 3 600 000 e `intentos` entre 1 y 99.
Se validan los enums del contrato. Los campos conocidos opcionales presentes
se validan también; no se exige `idPartida` a pista ni cerradura.
Los campos desconocidos se omiten, incluidos datos privados y atributos que
intenten reemplazar claves o metadatos calculados por la Lambda.
`tsObjeto`, si viene, debe ser una fecha real UTC con tres decimales y Z.

## Ítems, tiempos y TTL

La PK es `thingName` y la SK String es `<UTC con ms>#<lectura o evento>`.
Se prefiere `tsObjeto` para la fecha; si falta, se convierte `tsRegla`.
Una lectura guarda `tipo = lectura`. Un evento guarda `tipo = evento` y
`evento = <tipo del mensaje original>`; `tipoMensaje` no se almacena.
Se conservan los campos admitidos, `tsRegla` y `tsObjeto` solo si fue recibido.

`tsGuardado` (epoch ms) se toma inmediatamente antes de llamar al repositorio:
es la marca de la aplicación para el PutItem, no un timestamp de confirmación
emitido por DynamoDB. `expiraEn` (epoch s) se calcula desde esa recepción más
30 días, únicamente en lecturas. Eventos no tienen TTL y ninguno de estos
ítems lleva `categoriaAlerta`; las alertas son responsabilidad de vigilancia.

No se inventa `tsObjeto` ni se calcula una latencia objeto–guardado cuando
falta. Para esas muestras solo es posible estudiar regla–guardado. Comparar
objeto–guardado requiere además considerar la sincronización del reloj NTP.

**Riesgo de colisión conservado:** misma caja, fecha con ms y mismo tipo de
lectura o evento producen la misma PK/SK. PutItem reemplaza el ítem anterior,
incluso en reintentos. Las pruebas demuestran esa pérdida potencial y que
los eventos de distinto tipo no colisionan. No se añaden UUID, sufijos ni
condiciones nuevas: una solución necesita acuerdo del equipo y contrato
actualizado. Enviar lecturas iguales con fechas distintas sí crea ítems
distintos. Ver [semántica de PutItem](https://docs.aws.amazon.com/boto3/latest/reference/services/dynamodb/table/put_item.html).

## Respuestas y logging seguro

Éxito tras retornar PutItem: `recibido: true`, `persistido: true`,
`descartado: false`. Inválidos: `persistido: false`, `descartado: true`;
`recibido` indica si la entrada era un objeto JSON. No hay escrituras para
inválidos. Un fallo de persistencia lanza una excepción de texto fijo y no
confirma éxito; permite que la invocación falle y AWS gestione sus reintentos.
Una escritura fallida o con respuesta perdida no permite asegurar que no
exista ya un ítem; los reintentos pueden reemplazarlo con la clave actual.

Solo se registran identidad y tipos validados, recepción y persistencia.
Los descartes contienen motivos controlados. No se vuelca payload, contexto,
valores rechazados ni detalles de excepciones del SDK.

## Pruebas y paquete local

Desde la raíz, sin instalar dependencias de prueba:

```powershell
python -B -m unittest discover -s lambda-almacenamiento/tests -v
python -B lambda-almacenamiento/preparar_paquete.py
```

Se conserva unittest, disponible en Python 3.12 y usado en la iteración 0;
pytest no está instalado en el entorno revisado. La suite también puede ser
recogida por pytest si se dispone de él. Los tests no usan credenciales ni AWS.

Salida: `lambda-almacenamiento/dist/caja-enigmas-almacenamiento.zip`.
El empaquetador verifica sintaxis Python 3.12, integridad y lista de archivos.
El handler y los módulos quedan en la raíz; no incluye tests, ejemplos,
credenciales ni código Alexa. Son fuentes Python puras, sin binarios locales.
Se utiliza boto3 y sus dependencias incluidos por el runtime administrado de
Lambda, sin mezclar versiones parciales de dependencias en el ZIP. La versión
del SDK del runtime puede cambiar; si el equipo requiere fijarla, debe
empaquetar su conjunto completo y repetir las comprobaciones.
Referencia: [paquetes Python Lambda](https://docs.aws.amazon.com/lambda/latest/dg/python-package.html).

Las verificaciones locales del ZIP no acreditan una ejecución en AWS.

## Ejemplos MQTT y SQL del contrato

`ejemplos/mensajes.json` conserva seis eventos y dos lecturas, incluida solo
luz. Copiar solo `payload` al `tema`; `entradaLambda` ilustra la transformación
SQL para pruebas. El publicador no añade `thingName`, `tipoMensaje` ni
`tsRegla`: los añade la regla. Los timestamps son ficticios, no mediciones.
El ejemplo pista corresponde a la skill; no al ESP32.
Para lecturas se publica en `shadow/update`; AWS genera `update/accepted`.
No publicar directamente en el tema de respuesta.

SQL versión **2016-03-23**, prevalece sobre el fragmento antiguo del plan:

```sql
-- CajaEnigmasLecturas
SELECT state.reported.distanciaCm AS distanciaCm,
       state.reported.luzPorcentaje AS luzPorcentaje,
       'lectura' AS tipo, topic(3) AS thingName, timestamp() AS tsRegla
FROM '$aws/things/+/shadow/update/accepted'
WHERE NOT isUndefined(state.reported.distanciaCm)
   OR NOT isUndefined(state.reported.luzPorcentaje)

-- CajaEnigmasEventos
SELECT *, 'evento' AS tipoMensaje, topic(2) AS thingName, timestamp() AS tsRegla
FROM 'caja-enigmas/+/eventos'
```

## Pendiente manual en AWS

La infraestructura y la recepción mínima de iteración 0 fueron reportadas por
Mateo; este trabajo local no las vuelve a verificar ni despliega iteración 1.

1. Obtener las exportaciones JSON reales de tabla, reglas y rol pendientes.
   Contrastar SQL, permisos de invocación por regla y esquema del contrato.
2. Revisar manualmente el rol: logs y `dynamodb:PutItem` exclusivamente sobre
   `CajaEnigmasHistorico`. El permiso `DescribeTable` reportado no es usado
   por este código; justificarlo o ajustarlo manualmente.
3. Cargar el ZIP manualmente en `caja-enigmas-almacenamiento`, Python 3.12,
   handler `lambda_function.lambda_handler`, en us-east-1.
4. Configurar `TABLA_HISTORICO=CajaEnigmasHistorico` y
   `DIAS_RETENCION_LECTURAS=30` (entero positivo); mantener logs INFO.
5. Publicar válidos de cada tipo y lecturas de cada sensor; comprobar ítems,
   tipos, timestamps y TTL solo en lecturas. Coordinar los reportes ficticios
   de Shadow con Adrián para no interferir con la caja.
6. Publicar inválidos identificables y comprobar motivo de descarte y ausencia
   de escrituras; recopilar evidencias reales. Las pruebas unitarias no
   prueban permisos efectivos ni el recorrido IoT–Lambda–DynamoDB.
7. Registrar resultados y enlaces a evidencias en el plan de Mateo cuando
   corresponda. Medir integridad y latencia es trabajo de iteración 3.

No se han hecho despliegues ni cambios a recursos, Alexa, firmware o vigilancia.
