# Calibración y protocolo de ensayo

## Antes de tomar datos

1. Punzón por lo menos 1 cm por encima de la cúpula y nada tocando las celdas.
2. Shield: puente V en 3V3 y conector DC sin conectar.
3. Si cambiaste el código: subir `fsr_ensayo` (Upload Speed 115200, sin los 12 V).
4. Cerrar el Monitor Serie y el Serial Plotter de Arduino IDE.
5. Conectar el USB del ESP32.
6. Conectar la fuente de 12 V (siempre después del USB).
7. Página → Ensayo → ESP32 por USB → Conectar.
8. `s`: revisar «límites 40 N y 2500 mV», Rm 10 kΩ y modo suma.
9. Esperar ~5 min para que las celdas se estabilicen.
10. Sin carga: FSR en su base (~140 mV) y fuerza cerca de 0 N.
11. `t` (tara sin contacto).
12. Si moviste la cúpula o la plataforma: `cs` con la masa donde presiona el punzón.
13. Verificar: sin masa F ≈ 0, con masa F = m · 9,81, y presionar hacia abajo da F positiva.
14. `e`: el eje del motor queda duro.
15. +0,05 y −0,05 a 0,05 mm/s: + baja y − sube.
16. `h`: z = 0 en el toque.
17. `r 3 1 0 5`: 5 ciclos de acondicionamiento sin grabar.
18. Elegir el ensayo, marcar «Grabar automáticamente» e iniciar.
19. Al terminar: guardar el CSV y llevarlo a Análisis.
20. `d`, quitar los 12 V y después el USB.

La pestaña **Ensayo** de la página tiene esta lista con casillas y botones que mandan cada comando.

## Celdas de carga

El firmware trae un factor teórico (≈ 21 900 cuentas por newton para una celda de 10 kg y 1 mV/V), pero cada celda es distinta en ±15 %. Calibra una vez; el factor queda guardado en el ESP32.

1. Sin nada encima de la celda (o solo la plataforma vacía): `t`.
2. Pon una masa conocida, idealmente 500 g o 1 kg, pesada en una balanza: `c1 500` (en gramos).
3. Repite con la celda 2: `t`, masa, `c2 500`.

**Con la plataforma montada sobre las dos celdas (modo suma):** usa `cs`, que calibra las dos juntas.

1. Sin nada encima: `t`.
2. Pon la masa **justo donde presiona el punzón** (sobre la cúpula o una placa pequeña en ese punto): `cs 500`.
3. El firmware escala los dos factores para que F1 + F2 = masa · g, corrige el signo y dice cómo se reparte la carga entre las dos celdas. Es exacta para ese punto, que es donde siempre se mide.

El signo depende de qué extremo de cada barra queda fijo: si al presionar la fuerza sale negativa, la calibración lo corrige y no hace falta cambiar hilos.

**Alternativa:** calibra cada celda **antes** de montar la plataforma, con la masa sobre el extremo libre de esa celda. Después de montar la plataforma, verifica: pon la misma masa en el centro, en un borde y en el otro. F debe dar lo mismo (±1 %).

**Verificación de linealidad:** con 100 g, 200 g, 500 g y 1 kg, anota F. El error debe quedar por debajo de 0,5 % del valor.

## FSR402

Lo que se busca es la relación entre la fuerza real (celdas) y la lectura del FSR (voltaje → resistencia → conductancia), con su histéresis, deriva y repetibilidad dentro de la silicona.

### Antes de medir

- **Acondicionamiento:** los FSR cambian en los primeros ciclos. Corre 5 ciclos sin grabar: `r 3 1 0 5`.
- Espera 1 minuto sin carga.
- Anota la temperatura ambiente: el FSR y la silicona cambian con ella.

### Límite del FSR

El firmware deja de bajar cuando el FSR llega a `vl` (2500 mV por defecto) y empieza a descargar. Vale para los ciclos (`r`), los escalones (`p`) y los movimientos a mano (`m`). Así se puede pedir una profundidad o un número de escalones de sobra: el ensayo se detiene solo en el voltaje que se quiere.

### Fuerza mínima de detección

1. Contacto con `h` (las celdas detectan el toque y ese punto es z = 0).
2. Escalones finos hasta el límite del FSR: `p 0.05 99 3 0.2` (0,05 mm por escalón, pausa de 3 s, hasta 2500 mV).
3. En el análisis, la fuerza mínima es la de las celdas en el primer escalón donde el voltaje del FSR supera la línea base más el umbral de ruido (lo que reporta `fsr_lectura`).

### Ensayos sugeridos

| Ensayo | Comando | Para qué |
|---|---|---|
| Ciclos lentos | `r 3 0.2 5 5` | Curva principal y repetibilidad |
| Ciclos a varias velocidades | `r 3 0.1 2 3`, `r 3 0.5 2 3`, `r 3 2 2 3` | Efecto de la velocidad (viscoelasticidad) |
| Escalones hasta el límite del FSR | `p 0.05 99 3 0.2` | Puntos estáticos, deriva en cada escalón e histéresis |
| Sostenido | `p 3 1 300 0.5` | Deriva (creep) del FSR con la fuerza constante de la silicona |

Ajusta la profundidad para quedarte por debajo del límite de fuerza (`f 40` por defecto). Usa el banco virtual para estimar qué profundidad da la fuerza que buscas.

### Qué reportar

- Curva **F contra R_FSR** (log-log) y ajuste `R = A · F^(−n)`.
- Curva **conductancia (1/R) contra F** y su ajuste lineal (zona útil).
- **Histéresis:** máxima diferencia entre carga y descarga, en % del rango.
- **Repetibilidad:** desviación entre ciclos a la misma fuerza.
- **Deriva:** cambio de la lectura del FSR en el escalón sostenido (% por minuto).

La pestaña **Análisis** de la página y `analisis/analizar_ensayo.py` calculan todo esto a partir del CSV.
