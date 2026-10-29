# Cableado

La lista completa de los 43 cables, con calibre, color y terminal, está en [cables.csv](cables.csv). El esquemático interactivo está en la pestaña **Esquema** de la [página](https://luisponce15.github.io/banco-mano-robotica/).

## Cadena de potencia

```
Pared 110 V ─ X1 ─ X2 (IEC C14: F1 T4A + S1) ─ C01/C02/C03 ─ PS1 (L, N, ⏚)
PS1 +V ─ C10 ─ S2 (switch de palanca) ─ C12 ─ W1-1   (+12 V)
PS1 −V ─ C13 ─────────────────────────────── W2-1   (GND)

W1/W2 ─┬─ DRV-A VMOT/GND (C20/C21)
       ├─ DRV-B VMOT/GND (C22/C23)
       ├─ U1 buck 12→6 V (C24/C25) ─ W3 (+6 V) / W4 (GND servos)
       └─ U2 LM2596 12→5 V (C26/C27) ─ C40 ─ ESP32 VIN · C41 ─ W5 (GND lógica)
```

No hay fusible F2: la fuente tiene protección interna contra corto y sobrecarga.

## Mapa de Wago 221-415

| Borne | W1 · +12 V | W2 · GND | W3 · +6 V | W4 · GND servos | W5 · GND lógica |
|---|---|---|---|---|---|
| 1 | S2 (C12) | PS1 −V (C13) | U1 OUT+ (C30) | U1 OUT− (C31) | U2 OUT− (C41) |
| 2 | DRV-A VMOT (C20) | DRV-A GND (C21) | INA1 VIN+ (C32) | SV1 − (C33) | GND_PL placa (C42) |
| 3 | DRV-B VMOT (C22) | DRV-B GND (C23) | INA2 VIN+ (C34) | SV2 − (C35) | Puente desde W4-5 (C38) |
| 4 | U1 IN+ (C24) | U1 IN− (C25) | C3 + (C36) | C3 − (C37) | FC-A COM (C44) |
| 5 | U2 IN+ (C26) | U2 IN− (C27) | Reserva (mano) | Puente a W5-3 (C38) | FC-B COM (C45) |

W2, W4 y W5 deben tener continuidad entre sí: es la tierra común.

## Servos con INA219

Cada INA219 va **en serie con el + del servo**:

```
W3-2 ─ C32 ─ INA1 VIN+ │shunt│ INA1 VIN− ─ C60 ─ SV1 + (rojo)    SV1 − (café) ─ C33 ─ W4-2
W3-3 ─ C34 ─ INA2 VIN+ │shunt│ INA2 VIN− ─ C61 ─ SV2 + (rojo)    SV2 − (café) ─ C35 ─ W4-3
ESP32 3V3 / GND / GPIO21 / GPIO22 ─ C62 ─ INA1 ─ INA2 (bus en cadena)
GPIO16 ─ C52 ─ SV1 S          GPIO17 ─ C53 ─ SV2 S
```

## Entradas

```
FC-A: COM ─ C44 ─ W5-4     NC ─ C46 ─ GPIO18
FC-B: COM ─ C45 ─ W5-5     NC ─ C47 ─ GPIO19
BTN:  C   ─ C55 ─ GND_PL   NC ─ C54 ─ GPIO23
```

## Sentido de los servos

- **DS3240 (adelante, flexión):** ángulo positivo = recoge el tendón y flexiona el dedo; negativo = lo relaja hacia la posición inicial.
- **DS3235 (atrás, lateral):** servo a la derecha → dedo a la derecha; a la izquierda → dedo a la izquierda.
