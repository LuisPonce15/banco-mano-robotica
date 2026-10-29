# Lista de componentes (rev D · A4988 · fuente 12 V)

| Ref. | Componente | Cant. | Nota |
|---|---|---|---|
| X1 | Cable de poder con clavija de 3 patas, 3 × 18 AWG | 1 | La tercera pata (tierra) es obligatoria |
| X2 / F1 / S1 | Entrada IEC C14 con portafusible e interruptor de 2 polos | 1 | Fusible F1: vidrio lento 5×20 mm **T4 A 250 V** |
| PS1 | Fuente conmutada **12 V · 10 A** | 1 | Selector lateral en **110 V**, V.ADJ en 12,0 V |
| S2 | Switch de palanca ON/OFF **10 A 125 VAC** | 1 | Corta +12 V a todo el banco |
| BTN | Botón arcade (microswitch NC) | 1 | Paro por software en GPIO23 |
| — | ESP32 DevKit V1 | 1 | |
| DRV-A / DRV-B | **A4988** con disipador | 2 (+2 de repuesto) | 1/16 de paso |
| FAN | Ventilador 5 V, 40 mm | 1 | Encima de los A4988 |
| M1 / M2 | NEMA 17 (tipo 17HS4401, 1,7 A, 0,40 N·m) | 2 | Con varilla T8 de 8 mm de avance y acople 5×8 |
| SV1 | Servo **DS3240** | 1 | Flexión del dedo (adelante, polea del tendón) |
| SV2 | Servo **DS3235** | 1 | Desviación lateral (atrás) |
| U1 | Buck **12→6 V**, XL4016 (8 A o 20 A "300 W") | 1 | Potenciómetro de corriente (CC) al máximo |
| U2 | Buck **LM2596** 12→5 V | 1 | Alimenta el ESP32 por VIN (cable C40) |
| INA1 / INA2 | Módulo **INA219** (shunt R100 = 0,1 Ω) | 2 | Direcciones distintas: ver [ina219.md](ina219.md) |
| — | Resistencia shunt R050 2512 1 W (opcional) | 2 | Solo para medir bloqueos de más de 3,2 A |
| C1, C2 | Electrolítico low-ESR **100 µF 35–50 V** | 2 | En VMOT de cada A4988 |
| C3 | Electrolítico **1000–2200 µF 16 V** | 1 | Riel de 6 V de los servos |
| FC-A / FC-B | Microswitch con palanca (contacto NC) | 2 | |
| W1–W5 | Wago **221-415** | 5 | |
| — | Cable 16 / 18 / 20 / 22 / 24 AWG, dupont, termoencogible, marquillas | — | Ver [cables.csv](cables.csv) |
| — | Prensaestopa o sujetacable para X1, cubierta de bornes de PS1 | 1 | |

## Presupuesto de corriente (peor caso)

| Carga | Tensión | Corriente en su riel | Equivale en 12 V |
|---|---|---|---|
| M1 + M2 (A4988) | 12 V | ≤ 2 × 1,0 A (bobina) | ≈ 0,9 A |
| SV1 + SV2 | 6 V | ≤ 6 A pico | ≈ 3,4 A |
| ESP32 + lógica + ventilador | 5 V | ≤ 0,25 A | ≈ 0,12 A |
| **Total** | | | **≈ 4,5 A (54 W)** · PS1 da 10 A |
| Reserva: 4 × SG-5010 de la mano | 6 V | ≤ 5 A pico | ≈ 2,8 A → total ≈ 7,3 A |
