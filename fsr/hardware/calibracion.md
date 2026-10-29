# Calibración y protocolo de ensayo

## Celdas de carga

El firmware trae un factor teórico (≈ 21 900 cuentas por newton para una celda de 10 kg y 1 mV/V), pero cada celda es distinta en ±15 %. Calibra una vez; el factor queda guardado en el ESP32.

1. Sin nada encima de la celda (o solo la plataforma vacía): `t`.
2. Pon una masa conocida, idealmente 500 g o 1 kg, pesada en una balanza: `c1 500` (en gramos).
3. Repite con la celda 2: `t`, masa, `c2 500`.

**Modo suma:** calibra cada celda **antes** de montar la plataforma, con la masa sobre el extremo libre de esa celda. Después de montar la plataforma, verifica: pon la misma masa en el centro, en un borde y en el otro. F debe dar lo mismo (±1 %).

**Verificación de linealidad:** con 100 g, 200 g, 500 g y 1 kg, anota F. El error debe quedar por debajo de 0,5 % del valor.

## FSR402

Lo que se busca es la relación entre la fuerza real (celdas) y la lectura del FSR (voltaje → resistencia → conductancia), con su histéresis, deriva y repetibilidad dentro de la silicona.

### Antes de medir

- **Acondicionamiento:** los FSR cambian en los primeros ciclos. Corre 5 ciclos sin grabar: `r 3 1 0 5`.
- Espera 1 minuto sin carga.
- Anota la temperatura ambiente: el FSR y la silicona cambian con ella.

### Ensayos sugeridos

| Ensayo | Comando | Para qué |
|---|---|---|
| Ciclos lentos | `r 3 0.2 5 5` | Curva principal y repetibilidad |
| Ciclos a varias velocidades | `r 3 0.1 2 3`, `r 3 0.5 2 3`, `r 3 2 2 3` | Efecto de la velocidad (viscoelasticidad) |
| Escalones | `p 0.5 6 20 0.5` | Puntos estáticos, deriva en cada escalón e histéresis |
| Sostenido | `p 3 1 300 0.5` | Deriva (creep) del FSR con la fuerza constante de la silicona |

Ajusta la profundidad para quedarte por debajo del límite de fuerza (`f 40` por defecto). Usa el banco virtual para estimar qué profundidad da la fuerza que buscas.

### Qué reportar

- Curva **F contra R_FSR** (log-log) y ajuste `R = A · F^(−n)`.
- Curva **conductancia (1/R) contra F** y su ajuste lineal (zona útil).
- **Histéresis:** máxima diferencia entre carga y descarga, en % del rango.
- **Repetibilidad:** desviación entre ciclos a la misma fuerza.
- **Deriva:** cambio de la lectura del FSR en el escalón sostenido (% por minuto).

La pestaña **Análisis** de la página y `analisis/analizar_ensayo.py` calculan todo esto a partir del CSV.
