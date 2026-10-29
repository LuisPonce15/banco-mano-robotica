# Conexiones · banco FSR

Todo sale de un ESP32 propio, alimentado por el **USB del computador** (que además lleva los datos). El motor usa un **adaptador de 12 V** aparte. Los GND se unen en el A4988.

## Potencia

```
Adaptador 12 V (+) ─ P1 ─┬─ A4988 VMOT
                         └─ C1 100 µF (+)
Adaptador 12 V (−) ─ P2 ─┬─ A4988 GND (junto a VMOT)
                         └─ C1 100 µF (−)
ESP32 GND ─ L2 ─ A4988 GND (junto a VDD)      ← une las tierras
PC ═ USB ═ ESP32                             ← 5 V y datos
```

## A4988 (potenciómetro hacia arriba)

| Izquierda | Conexión | | Derecha | Conexión |
|---|---|---|---|---|
| EN | GPIO27 (S3) | | VMOT | +12 V (P1) + C1 |
| MS1 | 3,3 V | | GND | −12 V (P2) |
| MS2 | 3,3 V | | 2B | motor rojo |
| MS3 | 3,3 V | | 2A | motor azul |
| RST | puenteado con SLP **y a 3,3 V** | | 1A | motor verde |
| SLP | puenteado con RST | | 1B | motor negro |
| STEP | GPIO25 (S1) | | VDD | 3,3 V del ESP32 (L1) |
| DIR | GPIO26 (S2) | | GND | GND del ESP32 (L2) |

**Vref ≈ 0,8 V** con resistencias R100 (≈ 1 A). La fuerza que pide la silicona es pequeña: con varilla M8 y 1 A sobra torque.

> Revisa que **EN no quede puenteado con MS1** al soldar (es lo que falló en el banco del dedo).

## Celdas de carga → HX711

| Hilo de la celda | Borne del HX711 |
|---|---|
| Rojo | E+ |
| Negro | E− |
| Blanco | A− |
| Verde | A+ |

Si la fuerza sale negativa al presionar, no cambies cables: la calibración (`c1`, `c2`) corrige el signo sola.

## HX711 → ESP32

| HX711 | Celda 1 | Celda 2 |
|---|---|---|
| VCC (y VDD si lo tiene) | 3V3 | 3V3 |
| GND | GND | GND |
| DT (DOUT) | GPIO16 | GPIO18 |
| SCK (PD_SCK) | GPIO17 | GPIO19 |

Los HX711 van a **3,3 V**, no a 5 V: así su salida DT es de 3,3 V y no daña el ESP32. Las celdas son ratiométricas, así que la medida no cambia.

Por defecto el HX711 entrega **10 muestras/s**. Para 80 muestras/s, pon el pin **RATE** del chip a VCC (en muchos módulos hay un puente marcado RATE o una pista que va a GND y se corta).

## FSR402 (divisor de voltaje)

```
3V3 ─ F1 ─ FSR402 ─ F2 ─┬─ GPIO34
                        ├─ Rm 10 kΩ ─ GND     (F3)
                        └─ 100 nF ─ GND       (F4, opcional)

V = 3,3 V · Rm / (Rm + R_FSR)        R_FSR = Rm · (3,3 V − V) / V
```

- GPIO34 es del **ADC1**: sigue funcionando aunque el WiFi esté activo.
- Con **Rm = 10 kΩ** la curva es muy sensible con poca fuerza y se satura cerca de 3,2 V con fuerza alta. Con **3,3 kΩ** se aprovecha mejor el rango alto. La pestaña **Ensayo** deja comparar las dos antes de soldar.
- El ADC del ESP32 no es lineal cerca de 0 V ni de 3,3 V: el firmware usa `analogReadMilliVolts` (calibrado de fábrica) y promedia 8 lecturas.

## Lista de cables

Ver [cables.csv](cables.csv).
