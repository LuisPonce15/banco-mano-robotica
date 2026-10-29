# Pines

## ESP32 DevKit V1

| GPIO | Función | Va a | Cable |
|---|---|---|---|
| 25 | STEP eje A | DRV-A STEP | |
| 26 | DIR eje A | DRV-A DIR | |
| 32 | STEP eje B | DRV-B STEP | |
| 33 | DIR eje B | DRV-B DIR | |
| 27 | EN de ambos drivers (activo en LOW) | DRV-A EN y DRV-B EN | |
| 18 | Final de carrera A (`INPUT_PULLUP`, NC a GND) | FC-A NC | C46 |
| 19 | Final de carrera B (`INPUT_PULLUP`, NC a GND) | FC-B NC | C47 |
| 23 | BTN paro por software (`INPUT_PULLUP`, NC a GND) | BTN NC | C54 |
| 16 | PWM servo 1 (50 Hz, 500–2500 µs) | SV1 S · DS3240 | C52 |
| 17 | PWM servo 2 | SV2 S · DS3235 | C53 |
| 21 | I²C SDA | INA1 → INA2 | C62 |
| 22 | I²C SCL | INA1 → INA2 | C62 |
| VIN | 5 V desde U2 | U2 OUT+ | C40 |
| 3V3 | Lógica de drivers, MS1–MS3, VCC de INA | VDD DRV-A/B, INA | C43 |
| GND | Tierra lógica | W5 (GND_PL) | C42 |

En la serigrafía del DevKit, **D27 = GPIO27**, D25 = GPIO25, etc.

Con contacto NC a GND y `INPUT_PULLUP`, la entrada lee **LOW en reposo** y **HIGH cuando se presiona**. Si el cable se rompe también lee HIGH, así que una falla de cableado detiene la máquina en vez de dejarla sin protección.

## A4988

Con el potenciómetro hacia arriba:

| Izquierda | Conexión | | Derecha | Conexión |
|---|---|---|---|---|
| EN | GPIO27 | | VMOT | +12 V (W1-2 / W1-3) + C1/C2 100 µF |
| MS1 | 3,3 V | | GND | GND potencia (W2-2 / W2-3) |
| MS2 | 3,3 V | | 2B | Motor: rojo |
| MS3 | 3,3 V | | 2A | Motor: azul |
| RST | Puenteado con SLP **y a 3,3 V** | | 1A | Motor: verde |
| SLP | Puenteado con RST | | 1B | Motor: negro |
| STEP | GPIO25 (A) · GPIO32 (B) | | VDD | 3,3 V del ESP32 |
| DIR | GPIO26 (A) · GPIO33 (B) | | GND | GND lógica |

> **EN está justo al lado de MS1.** Si al soldar el puente de MS1–MS3 a 3,3 V también tocaste EN, el driver queda deshabilitado para siempre. Revisa con el multímetro que entre EN y MS1 no haya continuidad.

Colores de los cables del motor: **rojo/azul = bobina 2 (2B/2A)** y **verde/negro = bobina 1 (1A/1B)**. Confírmalo midiendo 1–4 Ω entre los hilos de cada par.
