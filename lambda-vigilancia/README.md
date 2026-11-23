# Lambda `caja-enigmas-vigilancia`

Es la lógica de la Nube de la Práctica 4 ([contrato, sección 9.3](../docs/contratos.md)). Cada 5 minutos revisa todas las cajas, y si una lleva más de `MINUTOS_SIN_REPORTE` sin lecturas, guarda una alerta en `CajaEnigmasHistorico`.

Va en la Nube porque una caja apagada o sin red no puede avisar de su propia desconexión.

## Qué decide

| Lo más nuevo de la caja en el histórico | Resultado |
|---|---|
| Una lectura de hace 15 min o menos | Nada: la caja está bien. |
| Una lectura de hace más de 15 min | Guarda una alerta, con `ultimaLectura` y `minutosSinReporte`. |
| Una alerta | Nada: ya se avisó, y no se repite hasta que la caja vuelva a reportar. |
| Nada (la caja nunca reportó, p. ej. `caja-enigmas-02`) | Guarda una alerta sin `ultimaLectura` ni `minutosSinReporte`. |

Solo cuentan las lecturas, no los eventos: una pista pedida por voz no prueba que la caja esté encendida.

La alerta es un ítem así:

```json
{ "thingName": "caja-enigmas-01", "ts": "2026-11-23T15:40:00.000Z#alerta",
  "tipo": "alerta", "categoriaAlerta": "sin_reporte", "tsGuardado": 1795794000000,
  "ultimaLectura": "2026-11-23T15:24:12.480Z", "minutosSinReporte": 15 }
```

`categoriaAlerta` es la clave del GSI disperso `alertas-por-fecha`, que usa el patrón de acceso 6 (alertas recientes de todas las cajas).

## Archivos

| Archivo | Responsabilidad |
|---|---|
| `lambda_function.py` | Lee las variables de entorno y arma las dependencias. |
| `vigilancia.py` | La regla: decide qué caja necesita una alerta y la arma. Lógica pura. |
| `servicios/inventario_things.py` | `ListThings`: todas las cajas con el prefijo, aunque nunca hayan reportado. |
| `servicios/repositorio_historico.py` | `Query` de la última lectura o alerta (patrón de acceso 5) y `PutItem` de la alerta. |
| `preparar_paquete.py` | Arma `dist/caja-enigmas-vigilancia.zip`. |

## Pruebas

Desde esta carpeta, sin AWS ni `boto3`:

```bash
python -m unittest discover -s tests
```

## Configuración en la consola (us-east-1)

Necesita la tabla `CajaEnigmasHistorico` con su GSI, que crea Mateo (contrato, sección 5.2).

1. **Rol** `caja-enigmas-vigilancia-rol`:
   - IAM → Roles → *Create role* → caso de uso *Lambda*;
   - agregar una política con el contenido de [config/aws/politica-rol-vigilancia.json](../config/aws/politica-rol-vigilancia.json).

   Permisos y por qué:
   - logs, solo de su propio grupo;
   - `iot:ListThings`, que no admite permisos por recurso y por eso va con `*`;
   - `dynamodb:Query` y `dynamodb:PutItem`, solo sobre la tabla de histórico.

   No puede leer ni cambiar los Shadows ni borrar nada.
2. **Función:**
   - Lambda → *Create function* → *Author from scratch*:
     - nombre `caja-enigmas-vigilancia`;
     - Python 3.12;
     - rol existente `caja-enigmas-vigilancia-rol`.
   - Generar el ZIP con `python preparar_paquete.py` y subirlo (*Upload from → .zip file*).
   - *Handler*: `lambda_function.lambda_handler`.
   - *Configuration → General*: 128 MB y *timeout* de 30 s.
   - *Configuration → Environment variables*:
     - `TABLA_HISTORICO=CajaEnigmasHistorico`;
     - `PREFIJO_THING=caja-enigmas-`;
     - `MINUTOS_SIN_REPORTE=15`.
3. **Disparador:**
   - *Add trigger* → *EventBridge (CloudWatch Events)* → *Create a new rule*;
   - nombre `caja-enigmas-vigilancia-cada-5-min`;
   - *Schedule expression* `rate(5 minutes)`.

   La consola agrega sola el permiso para que la regla invoque la función. La definición está en [config/aws/regla-vigilancia.json](../config/aws/regla-vigilancia.json).
4. **Probar:**
   - *Test*, con el evento `{}`, devuelve `{"cajasRevisadas": N, "alertasNuevas": [...]}`. La primera vez, `caja-enigmas-02` genera su alerta.
   - En DynamoDB, *Explore items* → índice `alertas-por-fecha` → `categoriaAlerta = sin_reporte`.

## Demo y tiempo de respuesta

En el peor caso, la alerta aparece 15 + 5 = 20 min después de la última lectura.

Para la demo:
- bajar `MINUTOS_SIN_REPORTE` a 3;
- cambiar la regla a `rate(1 minute)`;
- volver a los valores del contrato al terminar.

No hace falta tocar el código. Lo que se use en la demo se anota en el informe.

**Costo:** son 288 invocaciones por día (una cada 5 min) de menos de 1 s, dentro de la capa gratuita de Lambda. Cada una hace una `Query` corta por caja.
