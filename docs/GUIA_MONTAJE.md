# Montaje y diagnósticos del prototipo

El [montaje de la Práctica 4](#montaje-de-la-práctica-4) se describe después
del registro histórico de la Práctica 3.

El [informe](../INFORME.md) presenta los resultados finales. Esta guía conserva
el detalle práctico del montaje de mesa entregado y de los programas de
diagnóstico, sin duplicar sus conclusiones.

## 1. Montaje realizado

El equipo confirmó el montaje de la
[fotografía](evidencias/MontajeDeComponentes.jpg), basado en el
[circuito SVG](diagramas/circuito.svg): ESP32 DOIT DevKit V1, SG90, sensor
magnético de dos contactos e imán, protoboard, Dupont y USB de datos.
No hay carcasa ni pestillo acoplado; el servo se mueve sin carga de bloqueo.

| Terminal | Conexión documentada |
|---|---|
| USB del ESP32 | Computadora; única alimentación del montaje probado. |
| SG90 naranja | GPIO18. |
| SG90 rojo | VIN del ESP32 alimentado por USB. |
| SG90 marrón | GND de la fila inferior del ESP32. |
| Reed, cable 1 | a50; puente b50 hacia GPIO4. |
| Reed, cable 2 | a52; puente b52 hacia GND superior. |

a50/b50 forman un contacto; a52/b52, otro separado. Ambos GND comparten
referencia. El SVG no es un mapa físico de pines: las etiquetas de la placa
identifican los terminales. El sensor usa `INPUT_PULLUP`: LOW con imán cerca
representa tapa cerrada y HIGH con imán lejos, abierta. Los colores descritos
corresponden al montaje comunicado; no sustituyen la identificación del ejemplar.

Los [parámetros](../firmware/include/ConfiguracionHardware.h) son GPIO18,
GPIO4 y posiciones solicitadas de 45° cerrada / 135° abierta. Son consignas
del código, no una medición angular ni calibración de un pestillo. El firmware
integrado ordena cerrar al iniciar y mantiene PWM; no hace `detach()` automático.

## 2. Alimentación alternativa para una integración mecánica

La fuente regulada externa de 5 V y el capacitor no se instalaron en el
ensayo fotografiado. El contrato los conserva como alternativa para un montaje
con mayor demanda de corriente. Los pasos siguientes describen esa alternativa;
no se presentan como conexiones realizadas ni como tensión/corriente medidas.

Con USB y fuente desconectados, servo sin carga y eje libre:

| Paso | Conexión o revisión de la alternativa externa |
|---|---|
| S1 | Negativo de fuente identificada a GND del ESP32. |
| S2 | GND del servo a la referencia común. |
| S3 | Positivo externo al positivo del servo; separado de VIN, 3V3, GPIO y positivo USB. |
| S4, opcional | Capacitor + al positivo de alimentación del servo. |
| S5, opcional | Capacitor − a GND; comprobar polaridad y ausencia de cortocircuito. |
| S6 | Señal del servo a GPIO18; el GPIO no alimenta el motor. |

Se recomienda 470–1000 µF con tensión nominal superior a 5 V, preferiblemente
16 V. Esa tensión nominal no es la alimentación: se aplica la tensión de 5 V
del circuito. Si no se instala, se omiten S4 y S5 y se registra su ausencia.
El capacitor no reemplaza una fuente adecuada. No descargarlo con un
cortocircuito; verificar que no conserve tensión antes de manipularlo.

Identificar salida, regulación, polaridad y corriente declaradas del adaptador
y del servo. La inspección visual y los colores no miden tensión ni continuidad.
Esta última se mide sin energía. Utilizar instrumento y ayuda competente si
hay dudas; no abrir fuentes, manipular 220 V ni cortar cables de alimentación.
El criterio de corriente se obtiene de la documentación del ejemplar utilizado.

## 3. Diagnóstico del servo

Los programas están en [firmware/pruebas](../firmware/pruebas/README.md).
Cada carga reemplaza el firmware de la placa; no mezcla diagnósticos con la
aplicación integrada. Se cargan con los periféricos separados y luego se
desconecta la alimentación antes de cablear. Los comandos son una referencia
de operación, no acciones ejecutadas durante la revisión documental.

```powershell
pio run -d firmware/pruebas -e servo
pio run -d firmware/pruebas -e servo -t upload --upload-port COMx
pio device monitor -d firmware/pruebas -e servo --port COMx
```

El monitor usa 115200 baudios y envío de línea con Enter. Cerrar el monitor
antes de otra carga; seleccionar el puerto de la placa correcta.

| Comando | Comportamiento real del diagnóstico |
|---|---|
| Arranque | PWM inactivo; sin barrido ni movimiento automático. |
| `iniciar` | Activa PWM y solicita 90° de referencia, sin definir abierta/cerrada. Repetirlo activo no mueve. |
| `+` / `-` | Cambia 1°. |
| Entero | Acepta 45°–135°, con diferencia máxima de 5° y al menos 300 ms desde el movimiento anterior. |
| `estado` / `ayuda` | Muestra el último ángulo solicitado y el estado de PWM o la ayuda. |

El primer posicionamiento puede ser amplio: se realiza sin pestillo ni carga.
El mapeo de pulsos es el predeterminado de ESP32Servo, como en `CerraduraServo`.
El ángulo mostrado es una solicitud, no una medición del eje. La ventana limita
la exploración y no demuestra topes físicos seguros de cualquier SG90.
Ante atasco, calentamiento anormal o reinicios, detener y revisar sin energía.
Esperar que termine el movimiento antes del apagado normal. No recablear
energizado ni ordenar movimiento mientras una de las alimentaciones esté ausente.

Para acoplar un pestillo en una extensión: revisar la geometría sin energía,
explorar posiciones progresivas, evitar topes, fijar sin forzar y comprobar que
el cierre se sostenga sin par continuo. No usar 90° con un mecanismo acoplado
sin verificar recorrido libre; tampoco ampliar la ventana sin revisar el servo.
La configuración se concentra en `ConfiguracionHardware.h` y cualquier cambio
se coordina conforme a los contratos.

## 4. Diagnóstico del reed

Separar servo y su alimentación para la prueba aislada. Cargar `reed`, apagar
USB y realizar las conexiones: **R1**, un contacto a GND; **R2**, el otro a
GPIO4. No se alimenta el contacto seco desde 3V3 ni 5 V.

```powershell
pio run -d firmware/pruebas -e reed
pio run -d firmware/pruebas -e reed -t upload --upload-port COMx
pio device monitor -d firmware/pruebas -e reed --port COMx
```

El diagnóstico reutiliza `SensorTapa`: muestra lectura inicial, transiciones
muestreadas, cambios confirmados y resumen cada segundo, con marcas de
milisegundos. La referencia inicial no genera aviso. `huboCambio()` se consume
una vez; el filtro exige 50 ms de estabilidad muestreada y usa resta sin signo.
Serial puede omitir rebotes y añadir retardo, por lo que no mide con precisión
todos los rebotes eléctricos. El resumen de un segundo del diagnóstico no es
el respaldo de 60 s de la aplicación integrada, ni publica en AWS.

La fijación de imán y sensor a una caja futura debe conservar la interpretación
LOW/HIGH y proteger el componente de golpes y tensión en los cables. Los
[protocolos de medición](PRUEBAS_FISICAS.md) describen cómo registrar distancia,
repetibilidad y tiempos sin sustituirlos por resultados esperados.

## Montaje de la Práctica 4

Visto desde arriba, el montaje queda en el orden **protoboard 2 (sensores) →
ESP32 → protoboard 1 (LEDs y pulsadores)**. Las dos protoboards tienen 63 filas,
con grupos de contactos `a–e` y `f–j` separados por el canal central. El ESP32
DOIT DevKit V1 de 30 pines (ESP-WROOM-32, micro-USB) y el SG90 están fuera de las
protoboards. Se usaron cables macho-macho y macho-hembra. Esta sección registra
el cableado comunicado por el equipo y las tensiones teóricas del divisor.

| Diagrama | Alcance |
|---|---|
| [Circuito general](diagramas/circuito-general-p4.svg) | Vista superior de ambas protoboards, placa, servo y alimentación por USB. |
| [LEDs](diagramas/circuito-leds.svg) | Polaridad, resistencias y retornos. |
| [Pulsadores](diagramas/circuito-pulsadores.svg) | Contactos normalmente abiertos y `INPUT_PULLUP`. |
| [HC-SR04](diagramas/circuito-ultrasonico.svg) | Alimentación, TRIG y divisor de ECHO instalado. |
| [LDR](diagramas/circuito-ldr.svg) | Divisor de 3V3 y entrada ADC1. |
| [SG90](diagramas/circuito-servo.svg) | PWM, alimentación y posiciones ordenadas. |

### Alimentación y referencia común

El micro-USB alimenta el ESP32. El pin `VIN` va al riel rojo derecho de la
protoboard 2, compartido por el HC-SR04 y el SG90. `GND` del ESP32 va al riel
azul izquierdo de la protoboard 1, y un puente conecta ese riel con el azul
derecho de la protoboard 2. `3V3` va directamente a `i15` para la LDR. Los
positivos `VIN` y `3V3` pertenecen a redes distintas.

### Protoboard 1: LEDs y pulsadores

| LED | Jumper desde el ESP32 | Resistencia en serie | Ánodo | Cátodo y retorno al riel azul izquierdo |
|---|---|---|---|---|
| Rojo | D25/GPIO25 → c15 | 220 Ω, b15–b10 | a10 | a11 ↔ c11 → riel |
| Amarillo | D26/GPIO26 → c25 | 220 Ω, b25–b20 | a20 | a21 ↔ c21 → riel |
| Verde | D27/GPIO27 → c35 | 220 Ω, b35–b30 | a30 | a31 ↔ c31 → riel |

| Pulsador | Jumper de señal | Polo de señal | Polo de GND | Retorno al riel azul izquierdo |
|---|---|---|---|---|
| Rojo | D13/GPIO13 → c40 | e40–e42 | f40–f42 | h40 → riel |
| Amarillo | D14/GPIO14 → c45 | e45–e47 | f45–f47 | h45 → riel |
| Verde | RX2/GPIO16 → c50 | e50–e52 | f50–f52 | h50 → riel |
| Inicio | TX2/GPIO17 → c55 | e55–e57 | f55–f57 | h55 → riel |

Los pulsadores cruzan el canal central. Se dibujan con los dos polos separados
en reposo y unidos al presionar. El `INPUT_PULLUP` interno mantiene `HIGH` en
reposo y la entrada baja a `LOW` al cerrar el contacto. Esa es la orientación
eléctrica prevista para las patas indicadas; no se presenta como una medición
de continuidad.

### Protoboard 2: HC-SR04, LDR y SG90

| Terminal del HC-SR04 | Contacto | Conexión |
|---|---|---|
| VCC | a30 | b30 → riel rojo derecho (`VIN`) |
| TRIG | a31 | b31 → D23/GPIO23 |
| ECHO | a32 | R1 de 2,2 kΩ (b32–b38) en paralelo con R2 de 2,2 kΩ (c32–c38) |
| GND | a33 | e33 → riel azul derecho |

R1 y R2 equivalen a 1,1 kΩ entre ECHO y la fila 38. Desde esa fila sale el
jumper `e38 → D19/GPIO19` y baja R3 de 2,2 kΩ entre `d38` y `d43`.
`e43` conecta la fila 43 al riel azul derecho. Los transductores miran hacia
el exterior de la protoboard.

La LDR ocupa `g15–g17`. `3V3` llega a `i15` y `i17` lleva el punto medio a
`D34/GPIO34` (ADC1). La resistencia de 10 kΩ ocupa `h17–h22`, con retorno
`j22 → riel azul derecho`. Al iluminar la LDR, baja su resistencia y sube la
tensión en el punto medio.

El cable naranja del SG90 entra en `f47` y `h47` va a `D18/GPIO18`. El rojo
va al riel rojo derecho y el marrón al azul derecho. El firmware ordena por
PWM **45° al cerrar (incluido el arranque)** y **135° al abrir**. Son consignas
provisionales de [ConfiguracionHardware.h](../firmware/include/ConfiguracionHardware.h).
El montaje carece de un sensor independiente que confirme el ángulo alcanzado.

### Diferencias con el diseño de referencia y límites eléctricos

- El [contrato original, §8](contratos.md#8-hardware), describe 1 kΩ y 2 kΩ
  para ECHO. Se montaron tres resistencias de 2,2 kΩ. La razón nominal se
  mantiene en dos tercios: `5 × 2,2 / (1,1 + 2,2) ≈ 3,33 V`. Es un cálculo
  para 5 V nominales, sin medición que confirme el margen frente a tolerancias
  y variaciones de alimentación. ECHO llega a GPIO19 solo a través del divisor.
- El diseño recomienda una fuente externa de 5 V y el comentario de
  `PinServoCerradura` la da por supuesta. El montaje instalado toma toda la
  alimentación del USB y usa `VIN` para el HC-SR04 y el SG90. Los picos del servo
  pueden provocar caídas de tensión o reinicios. Una fuente externa regulada
  de 5 V con GND común es una mejora recomendada para el servo.
- Los GPIO y las consignas del servo coinciden con el firmware. El reed de P3
  queda fuera del montaje P4.
