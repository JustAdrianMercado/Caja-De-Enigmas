# Guía de montaje de la Caja de los Enigmas

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
| Rojo | D13/GPIO13 → c40 | e40–f40 | e42–f42 | h42 → riel |
| Amarillo | D14/GPIO14 → c45 | e45–f45 | e47–f47 | h47 → riel |
| Verde | RX2/GPIO16 → c50 | e50–f50 | e52–f52 | h52 → riel |
| Inicio | TX2/GPIO17 → c55 | e55–f55 | e57–f57 | h57 → riel |

Los pulsadores cruzan el canal central. Las patas de cada fila forman un polo
del pulsador. La señal llega a la fila superior y GND a la inferior. El
`INPUT_PULLUP` interno mantiene `HIGH` en reposo y el contacto entre ambas
filas lleva la entrada a `LOW` al presionar.

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
