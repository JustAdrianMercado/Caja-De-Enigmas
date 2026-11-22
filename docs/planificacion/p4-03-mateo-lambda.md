# Plan de Mateo Rojas — Almacenamiento, histórico de DynamoDB y backend de la skill

**Rama:** `feat/lambda-p4` · **Archivos a tu cargo:**
- `lambda-almacenamiento/**`;
- `lambda/**`, salvo `lambda/pistas.py`, que es de Lucía;
- en `config/aws/`: `tabla-historico.json`, `reglas/*.json`, `politica-rol-almacenamiento.json` y `politica-rol-lambda.json` (ampliada).

Ver los acuerdos en [p4-00-resumen.md](p4-00-resumen.md). Al cerrar cada iteración, llenás tu [registro](#registro-de-lo-aplicado) al final de este archivo.

Sos el responsable de que **todo lo que publica la caja quede guardado y bien validado**, y de que la skill entienda las órdenes nuevas del game master. No necesitás la caja para trabajar: el cliente de pruebas MQTT de la consola de IoT hace de objeto, y el Shadow de `caja-enigmas-02` se edita a mano.

## Iteración 0 — vie 20: tabla y cadena vacía funcionando

1. Los **patrones de acceso** y el diseño de la tabla ya están aprobados en [contratos.md, sección 5.2](../contratos.md). Los mockups de Lucía usan los mismos patrones.
2. **Tabla `CajaEnigmasHistorico`** (consola, *on-demand*):
   - PK `thingName` (S) y SK `ts` (S, ISO-8601 con ms);
   - GSI disperso para las alertas;
   - TTL sobre `expiraEn`.
   - Exportar la definición a `config/aws/tabla-historico.json`.
3. **`caja-enigmas-almacenamiento`** (Python 3.12) en su versión mínima, que solo escribe el evento en CloudWatch. Tiene su propio rol.
4. **Reglas de IoT** que la invocan:

   ```sql
   -- CajaEnigmasLecturas
   SELECT state.reported.distanciaCm AS distanciaCm,
          state.reported.luzPorcentaje AS luzPorcentaje,
          topic(3) AS thingName, timestamp() AS tsRegla
   FROM '$aws/things/+/shadow/update/accepted'
   WHERE NOT isUndefined(state.reported.distanciaCm)

   -- CajaEnigmasEventos
   SELECT *, topic(2) AS thingName, timestamp() AS tsRegla
   FROM 'caja-enigmas/+/eventos'
   ```

   El `thingName` sale del **tema**, no del payload. La política de IoT impide que una caja publique en el tema de otra.

**Listo cuando:** publicás desde el cliente MQTT el ejemplo de evento del contrato y aparece en los logs de la Lambda.

## Iteración 1 — sáb 21: Lambda de almacenamiento completa

Se organiza por responsabilidades, igual que `lambda/`. Por ejemplo:
- `lambda_function.py`: solo arma las dependencias;
- `validacion.py`: tipos y rangos de cada tipo de mensaje, según el contrato;
- `enriquecimiento.py`: `thingName`, `ts`, `tipo` y `expiraEn` en las lecturas;
- `servicios/repositorio_historico.py`: el `PutItem`.

Qué hace:
- **Descarta** los mensajes inválidos: tipo desconocido, campos faltantes o valores fuera de rango. Los registra en CloudWatch con el motivo y **no** los guarda (RF11).
- Las lecturas sin cambios también se guardan: cuentan para la integridad.
- Para `ts` usa `tsObjeto` si viene, y si no, `tsRegla`. Guarda los dos, para medir la latencia.

Además:
- **Tests** con pytest en `lambda-almacenamiento/tests/`: un mensaje válido de cada tipo, cada motivo de descarte y una tabla falsa (patrón de `lambda/tests/apoyo.py`).
- **Desplegar** el zip y probar con el cliente MQTT: ítems en DynamoDB, también desde el tema del Shadow.
- **Rol:** solo `dynamodb:PutItem` sobre la tabla de histórico y los logs. Exportarlo a `config/aws/politica-rol-almacenamiento.json`.

**Listo cuando:** los ejemplos válidos quedan en la tabla, los inválidos no, y los tests pasan.

### Orden de implementación local de la iteración 1

Plan pendiente de ejecutar, basado en el contrato v2 (§§2, 3.2, 5.2 y 6).
Se limita a `lambda-almacenamiento/**`; no modifica Alexa, firmware ni vigilancia.

1. **Validación (`validacion.py`).** Distinguir lecturas (`tipo = lectura`)
   de eventos (`tipoMensaje = evento`, conservando en `tipo` el nombre del
   evento). Validar `thingName` y `tsRegla` recibidos de las reglas; no derivar
   la caja de un campo del payload original. Usar el SQL del contrato §6,
   que prevalece sobre los fragmentos de la iteración 0 de este plan.
   - Lecturas: al menos `distanciaCm` (entero 2–400) o `luzPorcentaje`
     (entero 0–100); admitir solo luz y guardar también lecturas sin cambios.
   - Eventos: los seis tipos y sus campos obligatorios específicos de §3.2.
     Validar enums y rangos: `idPartida >= 1`, `duracionMs` 0–3 600 000,
     `intentos` 1–99, según corresponda. Rechazar booleanos como enteros.
   - Si viene `tsObjeto`, validar ISO-8601 UTC con milisegundos. Registrar
     motivos de descarte acotados, sin volcar payloads ni datos privados.
2. **Enriquecimiento (`enriquecimiento.py`).** Construir el ítem mediante
   una lista explícita de campos permitidos. Usar `tsObjeto` si existe y,
   en su ausencia, convertir `tsRegla` (epoch ms) a UTC con milisegundos.
   Formar `ts = <fecha>#lectura` o `<fecha>#<nombre del evento>`.
   En eventos guardar `tipo = evento` y `evento = <tipo original>`.
   Conservar `tsRegla` y, si vino, `tsObjeto`; incorporar `tsGuardado`
   (epoch ms) al escribir y `expiraEn` (epoch s, a 30 días) solo en lecturas.
   No agregar `categoriaAlerta` a lecturas ni eventos: el GSI es disperso.
3. **Persistencia (`servicios/repositorio_historico.py`).** Encapsular
   `PutItem` con tabla y reloj inyectables. No necesitar `DescribeTable`
   para procesar mensajes. Propagar errores de escritura sin convertirlos
   en confirmaciones de persistencia; separar esos errores de los descartes
   por validación. No cambiar las claves acordadas para resolver colisiones
   sin consultar al grupo y actualizar el contrato.
4. **Composición (`lambda_function.py`).** Conectar validación,
   enriquecimiento y repositorio; leer `TABLA_HISTORICO` y
   `DIAS_RETENCION_LECTURAS`. Mantener una entrada de prueba con dependencias
   falsas que no inicialice clientes AWS ni consulte credenciales.
5. **Pruebas locales con pytest (`tests/`).** Cubrir cada tipo válido,
   campos faltantes, tipos incorrectos, límites y enums inválidos;
   timestamps con y sin `tsObjeto`, TTL exclusivo de lecturas y separación
   `tipo`/`evento`. Usar una tabla falsa y un reloj fijo para comprobar que
   cada inválido produce cero escrituras y que los válidos llegan a `PutItem`.
   Incluir lecturas repetidas, errores del repositorio y la colisión de
   igual caja, timestamp y tipo bajo la clave actual. Ejecutar las pruebas
   y registrar resultados reales antes de declarar terminada la parte local.
6. **Preparación de integración.** Actualizar README y ejemplos; preparar
   el paquete local sin desplegar. Dejar para una actuación manual posterior
   la carga en AWS y la comprobación de válidos e inválidos en DynamoDB.
   La parte local no acredita por sí sola el cierre completo de iteración 1.

## Iteración 2 — dom 22: intents nuevos de la skill

**Servicio:** ampliar `ServicioShadow` ([lambda/servicios/servicio_shadow.py](../../lambda/servicios/servicio_shadow.py)) con métodos que escriban en `desired`: `pedir_modo`, `pedir_dificultad`, `pedir_parametro` y `pedir_partida`. Este último incrementa `solicitudPartida`.

**Handlers nuevos en `lambda/manejadores/`:**

| Intent | Comportamiento |
|---|---|
| `IniciarPartidaIntent` | En modo `juego`, incrementa `solicitudPartida`. En modo `manual`, explica que hay que pasar a modo juego. |
| `PedirPistaIntent` | Lee `reported.desafioActual`, toma una pista de `pistas.py` (Lucía) y publica un evento `pista` en `caja-enigmas/<thing>/eventos`. Así llega al histórico por la misma cadena validada. |
| `ConsultarProgresoIntent` | Responde con `estadoPartida` y `desafioActual`. |
| `CambiarModoIntent`, `ConsultarModoIntent` | `desired.modo` / `reported.modo`. |
| `CambiarDificultadIntent` | `desired.dificultad`. |
| `CambiarParametroIntent` | Valida el rango del contrato. Si el valor está fuera, lo rechaza explicando el rango y no escribe nada. |
| `ConsultarParametrosIntent` | La dificultad y los parámetros reportados. |
| `AbrirCerraduraIntent`, `CerrarCerraduraIntent` | **En modo `juego` se rechazan** con una explicación. Es la regla de convivencia del contrato. |

Además:
- **Quitar** `consultar_tapa.py`, su test y su registro en `lambda_function.py`.
- **Tests** de cada handler con el `ServicioShadow` falso que ya existe. Mientras Lucía no suba `pistas.py`, usar un diccionario propio en los tests.
- **Rol de la skill:** agregar `iot:Publish` solo sobre `caja-enigmas/*/eventos` y actualizar `config/aws/politica-rol-lambda.json`.
- **Probar con Lucía** contra `caja-enigmas-02`, con el `reported` editado a mano.

**Listo cuando:** desde el simulador se cambian el modo, la dificultad y un parámetro (y se ven en el `desired`), y "abre la caja" en modo juego se rechaza.

## Iteración 3 — lun 23: RNF del almacenamiento

| Prueba | Procedimiento |
|---|---|
| Integridad | Una ventana declarada (por ejemplo, 30 min de partidas). Contar los mensajes publicados, con el log Serial del ESP32 o el cliente MQTT, y los ítems guardados. Meta: ≥ 99 %. Publicar además 10 mensajes inválidos y comprobar que se descartan los 10. |
| Latencia del almacenamiento | Para cada ítem, la diferencia entre `tsObjeto` y la hora en que se escribió (registrada por la Lambda). Informar el máximo y el promedio. Meta: ≤ 5 s. |
| Muestras para la demo | Exportar un ejemplo de ítems de cada tipo a `docs/evidencias/` y preparar las consultas de la consola que se muestran en la demo. |
| Datos para el costo | Pasarle a Lucía las invocaciones por día de cada Lambda y las escrituras y lecturas de DynamoDB. |

## Iteración 4 — mar 24: informe

En `INFORME_P4.md`:
- **modelo de datos de DynamoDB**: tablas, atributos y tipos, claves, GSI, TTL y patrones de acceso;
- **diseño de las reglas** (SQL, tema y acción);
- **diagrama de clases de las Lambdas**;
- **secuencia del almacenamiento** (objeto → regla → Lambda → DynamoDB);
- configuración de las Lambdas y los roles, con la justificación de los permisos y los permisos mínimos de producción.

## Para la defensa: qué repasar de los demás

- `PartidaEnigmas` y el Shadow v2 de Adrián: cuándo se emite cada evento y qué pasa con los eventos sin conexión.
- La Lambda de vigilancia: por qué es programada y no una regla de IoT.
- Los desafíos y el circuito de Santiago.
- Los intents, los slots y los mockups de Lucía: de qué consulta sale cada dato.

## Registro de lo aplicado

Llenar al cerrar cada iteración, con lo que **de verdad** quedó hecho. Si un campo no aplica, "—". Qué va en cada campo y a qué sección del informe alimenta: [p4-00-resumen.md](p4-00-resumen.md#registro-de-lo-aplicado-en-cada-iteración).

### Iteración 0 — vie 20
- **Qué quedó funcionando:** según lo reportado por Mateo, la Lambda mínima
  `caja-enigmas-almacenamiento` fue creada y desplegada con Python 3.12 en
  `us-east-1`. Las reglas `CajaEnigmasLecturas` y `CajaEnigmasEventos` fueron
  creadas y probadas mediante MQTT y CloudWatch. La recepción no persiste
  mensajes en DynamoDB.
- **Archivos, commits y PR:** exportaciones JSON de recursos AWS pendientes
  de obtener realmente: `config/aws/tabla-historico.json`, las dos reglas
  en `config/aws/reglas/` y `config/aws/politica-rol-almacenamiento.json`.
  No se consignan commits ni PR como evidencia de este reporte.
- **Decisiones y por qué:** se documenta la configuración manual reportada;
  no se atribuyen decisiones ni justificaciones adicionales no suministradas.
- **Configuración en AWS (sin secretos):** reportada por Mateo en `us-east-1`:
  - Tabla `CajaEnigmasHistorico`, PK `thingName` (String), SK `ts` (String),
    capacidad bajo demanda.
  - GSI `alertas-por-fecha`, PK `categoriaAlerta` (String), SK `ts` (String),
    proyección `ALL`.
  - TTL sobre `expiraEn` activado.
  - Rol de almacenamiento con permisos de CloudWatch y política inline
    `CajaEnigmasHistoricoEscritura` con `dynamodb:PutItem` y
    `dynamodb:DescribeTable`.
  - Lambda y las dos reglas indicadas en «Qué quedó funcionando».
- **Pruebas y valores medidos:** según el reporte, ambas reglas se probaron
  mediante MQTT y CloudWatch. En la prueba de eventos, el registro mostró
  `thingName = caja-enigmas-02`, `tipoMensaje = evento`,
  `tipo = desafio_superado` y `persistido = false`. No se reportan cantidades
  de ejecuciones, latencias ni resultados de escritura en DynamoDB.
- **Evidencias:** Mateo refiere capturas o logs de respaldo de los hechos
  anteriores. Este registro transcribe su reporte; no constituye una revisión
  directa de AWS ni una inspección independiente de esas capturas o logs.
  Pendiente incorporar o enlazar los respaldos reales en el repositorio;
  no se inventan nombres de archivo, rutas ni identificadores de registros.
- **Cambios al contrato o al plan:** no se modifica el contrato. El permiso
  `dynamodb:DescribeTable` reportado es adicional al mínimo del contrato §6
  (logs y `dynamodb:PutItem`); queda pendiente justificarlo o que una persona
  ajuste manualmente el rol. No se afirma que se haya hecho ese ajuste.
- **Problemas y pendientes:** el criterio funcional de recepción de la
  iteración 0 se considera cubierto según el reporte de Mateo. Para el cierre
  formal faltan las exportaciones JSON reales, la incorporación o enlaces de
  las evidencias y la revisión documentada de los permisos frente al contrato.
  El SQL exacto y los permisos basados en recursos podrán contrastarse con
  las configuraciones reales; no se declaran verificados por este reporte.
  La validación y persistencia completas pertenecen a la iteración 1.

### Iteración 1 — sáb 21
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 2 — dom 22
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 3 — lun 23
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Configuración en AWS (sin secretos):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**
