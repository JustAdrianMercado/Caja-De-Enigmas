# Plan de Santiago Torres — Montaje, clases del hardware y los tres desafíos

**Rama:** `feat/hardware-p4` · **Archivos a tu cargo:**
- en `firmware/include|src/`: `Pulsador.*`, `IndicadorLeds.*`, `SensorDistancia.*`, `SensorLuz.*`, `DesafioMemoria.*`, `DesafioDistancia.*`, `DesafioLuz.*`, `CerraduraServo.*` (se conserva) y `SensorTapa.*` (para borrarlo);
- `firmware/include/ConfiguracionHardware.h`;
- `firmware/pruebas/**`;
- `docs/diagramas/`, `docs/GUIA_MONTAJE.md` y las evidencias de montaje y pruebas en `docs/evidencias/p4/`.

Ver los acuerdos en [p4-00-resumen.md](p4-00-resumen.md). Al cerrar cada iteración, llenás tu [registro](#registro-de-lo-aplicado) al final de este archivo.

Sos el responsable de que la caja exista físicamente y de que **los tres desafíos funcionen por sí solos**, sin red y sin AWS. Adrián se encarga de encadenarlos en una partida (`PartidaEnigmas`) y de la conexión con AWS. Tus desafíos implementan la interfaz `Desafio.h`, que él publica el viernes.

## Materiales

- ESP32 DOIT DevKit V1, servo SG90, 3 LEDs (rojo, amarillo y verde), 4 pulsadores, HC-SR04, LDR, protoboard y cables.
- Resistencias:
  - 3 de 220 Ω (LEDs);
  - 1 de 10 kΩ (divisor de la LDR);
  - 1 kΩ y 2 kΩ para bajar el ECHO del HC-SR04 de 5 V a 3,3 V.
- Fuente de 5 V externa con GND común con el ESP32, o como mínimo un capacitor de 470–1000 µF. Con el servo y el HC-SR04 en VIN aumentan los picos de corriente.
- La caja de cartón rígido o MDF de la P3.
- **Ya no se usa el reed switch.** Adrián ya lo quitó de `main.cpp` y de `EstadoShadow`. Vos borrás, en un mismo PR, `SensorTapa.*`, `firmware/pruebas/reed` y su entorno `reed` en `firmware/pruebas/platformio.ini`, porque ese diagnóstico es lo único que todavía usa la clase. También quitás el reed del circuito y de la guía.

## Iteración 0 — vie 20: contrato y pines

1. Los pines y los rangos ya están aprobados en [contratos.md, sección 8](../contratos.md). Si al montar hace falta cambiar un pin, o las primeras mediciones piden otros rangos, actualizar el contrato en el mismo PR. Los valores por defecto de distancia (20 cm) y luz (70 %) son provisionales: calibrarlos en la iteración 1.
2. Revisar `Desafio.h` y `ParametrosJuego.h` en el PR de Adrián, antes de que se integren. Sobre todo:
   - la regla de que **ningún desafío bloquea `loop()`**: nada de `delay()`, y `pulseIn` con un límite de unos 25 ms;
   - el método `detener()`, que se agregó al contrato: se llama al abandonar la partida y apaga los LEDs del desafío.
3. Actualizar `ConfiguracionHardware.h` con los pines nuevos y quitar `PinReedTapa`.
4. Armar en la protoboard los LEDs y los pulsadores, que son lo más rápido de probar.

**Listo cuando:** los pines están en el contrato y en `ConfiguracionHardware.h`, y `main` compila.

## Iteración 1 — sáb 21: clases de los componentes

Una clase por componente, cada una con un diagnóstico propio en `firmware/pruebas/<componente>/main.cpp`, con el mismo patrón que `pruebas/servo` y un entorno en `firmware/pruebas/platformio.ini`.

| Clase | Responsabilidad | Detalle |
|---|---|---|
| `Pulsador` | Lee un pulsador con antirrebote y detecta la pulsación. | Reutilizar el antirrebote de 50 ms de `SensorTapa.cpp`, y después borrar `SensorTapa.*` (ver Materiales). Método `fuePulsado()`. |
| `IndicadorLeds` | Enciende, apaga y hace parpadear los 3 LEDs sin bloquear. | Necesita `actualizar()` con `millis()` para mostrar una secuencia o un parpadeo de éxito o de fallo. |
| `SensorDistancia` | Mide en cm con el HC-SR04. | Una medición cada ~100 ms; `pulseIn` con un límite de ~25 ms; descartar las lecturas fuera de rango (por ejemplo, < 2 cm o > 200 cm). |
| `SensorLuz` | Lee la LDR y la convierte a 0–100 %. | ADC1 (GPIO 34). Promedio de varias muestras. Calibrar el 0 % (tapada) y el 100 % (linterna). |

**Listo cuando:**
- cada diagnóstico muestra por Serial lo que lee o hace cada componente;
- el HC-SR04 mide una mano a 10, 20 y 30 cm con un error de ±2 cm (medido con una regla);
- la LDR distingue la linterna de la luz del ambiente.

## Iteración 2 — dom 22: los tres desafíos e integración con Adrián

Cada desafío implementa `Desafio` (`iniciar(parametros)`, `actualizar()`, `detener()`, `resultado()`, `nombre()`) y se encarga de sus propios LEDs.

| Desafío | `nombre()` | Cómo se supera | Cuándo falla |
|---|---|---|---|
| `DesafioMemoria` | `"memoria"` | Muestra una secuencia aleatoria de `longitudSecuencia` colores y el jugador la repite con los 3 pulsadores. | Un pulsador equivocado. `PartidaEnigmas` cuenta el intento y lo reinicia. |
| `DesafioDistancia` | `"distancia"` | La mano se mantiene a `distanciaObjetivoCm ± toleranciaCm` durante un tiempo fijo (por ejemplo, 2 s, como constante con nombre). Los LEDs pueden guiar: rojo lejos, amarillo cerca, verde en el objetivo. | No falla. Solo sigue `EnCurso` hasta que se cumple. |
| `DesafioLuz` | `"luz"` | `luzPorcentaje ≥ umbralLuzPorcentaje` durante un tiempo fijo (por ejemplo, 1 s). | No falla. |

1. **Mañana:** un programa `firmware/pruebas/desafios/main.cpp` que corre los tres desafíos en orden, sin red, con el pulsador de inicio. Sirve para probarlos sin esperar el firmware de Adrián.
2. **Tarde, con Adrián y la placa:** él integra tus clases en `main.cpp` en su rama. Vos revisás el PR y prueban juntos una partida completa en la caja.

**Listo cuando:**
- una partida completa abre el servo en la caja real;
- un error en la memoria reinicia ese desafío;
- el juego sigue funcionando con el WiFi apagado.

## Iteración 3 — lun 23: pruebas físicas y del Edge

| Prueba | Procedimiento |
|---|---|
| Exactitud del HC-SR04 | 10 mediciones a 10, 20 y 30 cm contra una regla. Tabla con el error medio y el máximo. |
| Exactitud y calibración de la LDR | Valores en oscuridad, con luz ambiente y con linterna a 10 cm, 10 veces cada uno. Justifica el umbral por defecto. |
| Tiempo de respuesta del Edge | Desde que se cumple la condición del último desafío hasta que el servo se mueve, 10 veces. Con Adrián, usando las marcas de `millis()` por Serial. |
| Estabilidad | ≥ 10 min de partidas seguidas sin reinicios, con el servo y el HC-SR04 activos. |

Además: fotos del montaje en `docs/evidencias/p4/montaje/`, `docs/diagramas/circuito-general-p4.svg` y la `GUIA_MONTAJE.md` de la P4.

## Iteración 4 — mar 24: informe

En `INFORME_P4.md`, en tus secciones:
- **diagrama de bloques** del objeto y **diagrama de circuito**;
- justificación de la cantidad y el tipo de sensores y actuadores. **Incluir la justificación del HC-SR04**, porque la consigna lo restringe: el desafío necesita la distancia en cm sin contacto, y un sensor IR de obstáculo solo detecta presencia;
- tablas de las pruebas físicas;
- clases del hardware en el diagrama de clases, junto con Adrián.

## Para la defensa: qué repasar de los demás

- `PartidaEnigmas` de Adrián: estados, cómo cambia de desafío, qué pasa en modo `manual` y sin red.
- El Shadow v2, el tema de eventos y la Lambda de vigilancia (por qué la "caja sin reportar" no puede ir en el Edge).
- La Lambda de almacenamiento de Mateo: qué valida, qué descarta y cómo es la tabla de histórico.
- La skill de Lucía: intents nuevos, cómo pide una pista y cómo cambia la dificultad.

## Registro de lo aplicado

Llenar al cerrar cada iteración, con lo que **de verdad** quedó hecho. Si un campo no aplica, "—". Qué va en cada campo y a qué sección del informe alimenta: [p4-00-resumen.md](p4-00-resumen.md#registro-de-lo-aplicado-en-cada-iteración).

### Iteración 0 — vie 20
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Montaje y calibración (pines, valores, alimentación):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 1 — sáb 21
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Montaje y calibración (pines, valores, alimentación):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 2 — dom 22
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Montaje y calibración (pines, valores, alimentación):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**

### Iteración 3 — lun 23
- **Qué quedó funcionando:**
- **Archivos, commits y PR:**
- **Decisiones y por qué:**
- **Montaje y calibración (pines, valores, alimentación):**
- **Pruebas y valores medidos:**
- **Evidencias:**
- **Cambios al contrato o al plan:**
- **Problemas y pendientes:**
