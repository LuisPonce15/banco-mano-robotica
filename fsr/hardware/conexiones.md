# Conexiones · banco FSR

Todo sale de un **ESP32-S de 38 pines** propio (NodeMCU-32S / DevKitC), montado sobre un **base shield** (placa de expansión) que repite cada pin en una fila **G · V · S**. El ESP32 se alimenta por el **USB del computador**, que además lleva los datos. El motor usa un **adaptador de 12 V** aparte, conectado directo al A4988. Los GND se unen en el A4988.

## El shield

Cada GPIO del shield tiene tres pines:

| Pin | Qué es |
|---|---|
| **G** | GND |
| **V** | Alimentación: 3,3 V o 5 V según el **puente de selección** del shield |
| **S** | La señal del GPIO |

Antes de conectar nada:

1. **Pon el puente de voltaje del shield en 3V3** y mide entre V y G de cualquier fila: debe dar **3,3 V**. Si da 5 V, no conectes los HX711 ni el FSR a V: el pin DT del HX711 y GPIO34 no aguantan 5 V. En ese caso usa los pines de la fila **3V3**.
2. **No conectes el conector DC del shield.** El ESP32 ya recibe 5 V por USB; dos fuentes de 5 V unidas pueden devolver corriente al computador. El adaptador de 12 V va solo al A4988.
3. Revisa que el shield sea del **ancho** de tu ESP32-S (hay placas de 25,4 mm y de 22,9 mm entre filas) y que el ESP32 entre con el USB hacia el borde del shield.
4. El orden G · V · S cambia entre fabricantes: sigue la **serigrafía** de tu shield.

## Pines del ESP32-S de 38 pines

Vista desde arriba, con la antena arriba y el USB abajo. En **negrita**, los que usa este banco.

| # | Izquierda | Uso | | Derecha | Uso |
|---|---|---|---|---|---|
| 1 | **3V3** | A4988 VDD (L1) | | **GND** | — |
| 2 | EN | — | | GPIO23 | libre |
| 3 | GPIO36 (VP) | libre | | GPIO22 | libre |
| 4 | GPIO39 (VN) | libre | | TX0 | USB |
| 5 | **GPIO34** | FSR: V → FSR, S → nodo, G → Rm (F1–F4) | | RX0 | USB |
| 6 | GPIO35 | libre | | GPIO21 | libre |
| 7 | GPIO32 | libre | | GND | — |
| 8 | GPIO33 | libre | | **GPIO19** | HX711-2 SCK (H8) |
| 9 | **GPIO25** | A4988 STEP (S1) · G → A4988 GND (L2) | | **GPIO18** | HX711-2 DT, VCC, GND (H5–H7) |
| 10 | **GPIO26** | A4988 DIR (S2) | | GPIO5 | libre (arranque) |
| 11 | **GPIO27** | A4988 EN (S3) | | **GPIO17** | HX711-1 SCK (H4) |
| 12 | GPIO14 | libre | | **GPIO16** | HX711-1 DT, VCC, GND (H1–H3) |
| 13 | GPIO12 | no usar (arranque) | | GPIO4 | libre |
| 14 | GND | — | | GPIO0 | no usar (BOOT) |
| 15 | GPIO13 | libre | | GPIO2 | no usar (arranque, LED) |
| 16 | SD2 | no usar (flash) | | GPIO15 | no usar (arranque) |
| 17 | SD3 | no usar (flash) | | SD1 | no usar (flash) |
| 18 | CMD | no usar (flash) | | SD0 | no usar (flash) |
| 19 | 5V | — | | CLK | no usar (flash) |

Los pines son los mismos del firmware: **no hay que cambiar el código**. En Arduino IDE la placa sigue siendo **ESP32 Dev Module** (o **NodeMCU-32S**).

## Si usas el ESP32 DevKit V1 de 30 pines

Mismos GPIO y mismo firmware; solo cambia dónde están en la placa (antena arriba, USB abajo):

| # | Izquierda | Uso | | Derecha | Uso |
|---|---|---|---|---|---|
| 4 | **D34** | FSR (F1–F4) | | RX0 | USB |
| 8 | **D25** | A4988 STEP · GND (G de la fila) | | **D18** | HX711-2 DT, VCC, GND |
| 9 | **D26** | A4988 DIR | | **TX2 (17)** | HX711-1 SCK |
| 10 | **D27** | A4988 EN | | **RX2 (16)** | HX711-1 DT, VCC, GND |
| 6 | D32 | libre | | **D19** | HX711-2 SCK |
| 14 | GND | GND (sin shield) | | GND | GND (sin shield) |
| 15 | VIN | no usar | | **3V3** | A4988 VDD |

**Sin shield** (dupont o protoboard) el ESP32 tiene un solo pin 3V3: reparte 3V3 y GND con una protoboard o un Wago hacia el A4988 (VDD), los dos HX711 (VCC) y el FSR. Nunca alimentes estos módulos desde VIN o 5V.

La pestaña **Conexiones** de la página tiene un selector de placa (30 o 38 pines) y de montaje (con o sin shield), con el pineado y la lista de chequeo de cada combinación.

## A4988: conexión mínima

La conexión típica de las guías (por ejemplo, Last Minute Engineers con un Arduino UNO) usa solo STEP, DIR, VDD, GND, VMOT, GND, las 4 bobinas y un puente RST–SLP. Con el ESP32:

- VDD va a **3V3**, no a 5 V, para que el A4988 entienda las señales de 3,3 V del ESP32.
- STEP → GPIO25 y DIR → GPIO26 (en el UNO eran los pines 3 y 2).
- Si dejas **EN libre**, el motor queda siempre con fuerza y el comando `d` no lo suelta.
- Si dejas **MS1–MS3 libres**, el driver da paso completo: pon `MICROSTEP = 1` en el firmware.
- Mide SLP: si da menos de 2,3 V, tu módulo no trae el pull-up; une RST + SLP a VDD (fue la falla del banco del dedo).

El banco usa la conexión completa (EN a GPIO27, MS1–MS3 a VDD, RST + SLP a VDD) para poder soltar el motor y tener 1/16 de paso.

## Cómo queda cada módulo en el shield

```
A4988            ← fila GPIO25: S = STEP, G = GND lógica
                 ← fila GPIO26: S = DIR
                 ← fila GPIO27: S = EN
                 ← fila 3V3:      VDD (y en el driver: VDD → MS1–MS3 y RST+SLP)

HX711-1 (celda 1) ← fila GPIO16: G = GND, V = VCC, S = DT
                  ← fila GPIO17: S = SCK

HX711-2 (celda 2) ← fila GPIO18: G = GND, V = VCC, S = DT
                  ← fila GPIO19: S = SCK

FSR402            ← fila GPIO34: V → FSR → S ; Rm entre S y G ; 100 nF entre S y G
```

Cada HX711 sale con un cable de 4 hilos: tres de la fila del DT (G, V, S) y uno de la fila del SCK.

## Potencia

```
Adaptador 12 V (+) ─ P1 ─┬─ A4988 VMOT
                         └─ C1 100 µF (+)
Adaptador 12 V (−) ─ P2 ─┬─ A4988 GND (junto a VMOT)
                         └─ C1 100 µF (−)
Shield fila 25 · G ─ L2 ─ A4988 GND (junto a VDD)    ← une las tierras
PC ═ USB ═ ESP32                                    ← 5 V y datos
Conector DC del shield: sin conectar
```

## A4988 (potenciómetro hacia arriba)

| Izquierda | Conexión | | Derecha | Conexión |
|---|---|---|---|---|
| EN | fila GPIO27 · S (S3) | | VMOT | +12 V (P1) + C1 |
| MS1 | 3,3 V | | GND | −12 V (P2) |
| MS2 | 3,3 V | | 2B | motor rojo |
| MS3 | 3,3 V | | 2A | motor azul |
| RST | puenteado con SLP **y a 3,3 V** | | 1A | motor verde |
| SLP | puenteado con RST | | 1B | motor negro |
| STEP | fila GPIO25 · S (S1) | | VDD | fila 3V3 (L1) |
| DIR | fila GPIO26 · S (S2) | | GND | fila GPIO25 · G (L2) |

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

## HX711 → shield

| HX711 | Celda 1 | Celda 2 |
|---|---|---|
| VCC (y VDD si lo tiene) | fila GPIO16 · V | fila GPIO18 · V |
| GND | fila GPIO16 · G | fila GPIO18 · G |
| DT (DOUT) | fila GPIO16 · S | fila GPIO18 · S |
| SCK (PD_SCK) | fila GPIO17 · S | fila GPIO19 · S |

Los HX711 van a **3,3 V** (puente V del shield en 3V3), no a 5 V: así su salida DT es de 3,3 V y no daña el ESP32. Las celdas son ratiométricas, así que la medida no cambia.

Por defecto el HX711 entrega **10 muestras/s**. Para 80 muestras/s, pon el pin **RATE** del chip a VCC (en muchos módulos hay un puente marcado RATE o una pista que va a GND y se corta).

## FSR402 (divisor de voltaje, todo en la fila GPIO34)

```
fila 34 · V (3,3 V) ─ F1 ─ FSR402 ─ F2 ─┬─ fila 34 · S (GPIO34)
                                        ├─ Rm 10 kΩ ─ fila 34 · G   (F3)
                                        └─ 100 nF ─ fila 34 · G     (F4, opcional)

V = 3,3 V · Rm / (Rm + R_FSR)        R_FSR = Rm · (3,3 V − V) / V
```

- GPIO34 es del **ADC1**: sigue funcionando aunque el WiFi esté activo.
- Rm y el condensador van entre S y G de la misma fila: se pueden soldar a un conector dupont de 3 pines o poner en una protoboard pequeña.
- Con **Rm = 10 kΩ** la curva es muy sensible con poca fuerza y se satura cerca de 3,2 V con fuerza alta. Con **3,3 kΩ** se aprovecha mejor el rango alto. La pestaña **Conexiones** de la página deja comparar antes de soldar.
- El ADC del ESP32 no es lineal cerca de 0 V ni de 3,3 V: el firmware usa `analogReadMilliVolts` (calibrado de fábrica) y promedia 8 lecturas.

## Lista de cables

Ver [cables.csv](cables.csv).
