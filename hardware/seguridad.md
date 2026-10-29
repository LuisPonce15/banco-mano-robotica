# Seguridad

## USB y fuente al mismo tiempo

El ESP32 recibe 5 V por **VIN** desde el buck U2 (cable **C40**). El USB del computador también le da 5 V. Si los dos están conectados a la vez, las dos fuentes de 5 V quedan unidas a través de la placa.

**Regla:** cuando el computador esté conectado por USB, **C40 va desconectado** (saca ese cable del VIN). El resto del banco puede seguir encendido: la tierra es común por GND_PL, así que el ESP32 manda STEP/DIR/EN a los drivers sin problema.

## Orden de encendido

1. USB conectado (con C40 desconectado) o C40 conectado (sin USB).
2. S1 en ON (enchufe y módulo IEC): la fuente enciende, S2 sigue en OFF.
3. S2 en ON: llegan 12 V a los drivers y a los bucks.

## Orden de apagado

1. **S2 en OFF** (corta los 12 V).
2. **S1 en OFF** o desenchufar.
3. **USB** al final.

## Energización por etapas (primera vez o después de cambios)

| Etapa | Conectado | Medir | Esperado |
|---|---|---|---|
| 1 | Solo PS1 (S2 en OFF) | +V / −V | 12,0 V ± 0,2 |
| 2 | S2 en ON, cargas abiertas | W1 / W2 | 12,0 V |
| 3 | + U1 (sin servos ni C3) | U1 OUT en vacío | 6,0 V |
| 4 | + U2 (sin ESP32) | U2 OUT | 5,0 V |
| 5 | + ESP32 | 3V3 / GND | 3,3 V |
| 6 | + DRV-A/B, sin motores | Vref | ≈ 0,8 V |
| 7 | Apagar, conectar M1 y M2, encender | Prueba de giro | Ambos ejes iguales |
| 8 | S2 a OFF en movimiento | | Todo se detiene, LED de PS1 sigue encendido |
| 9 | Apagar, conectar C3, SV1, SV2 | W3 / W4 con servos moviéndose | ≥ 5,7 V en picos |

## Sin energía (multímetro)

- L–N en la clavija con S1 encendido: valor alto o en kΩ, nunca 0 Ω.
- L–PE y N–PE: abierto (OL).
- PE de la clavija ↔ FG de PS1: menos de 1 Ω.
- W1 ↔ W2, W3 ↔ W4 y VIN ↔ GND del ESP32: no deben estar en corto.
- W2 ↔ W4 ↔ W5: continuidad (tierra común).
- Polaridad de C1, C2 y C3: la raya va hacia GND.

## Paros

- **S2** (switch de palanca): corte físico de los 12 V. Se queda en la posición donde lo dejas.
- **BTN** (botón arcade): paro por software. El firmware pone EN en HIGH y deja de mover. La fuente sigue encendida. Para volver: soltar el botón y rearmar desde la página.
