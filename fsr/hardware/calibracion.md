# Calibración y protocolo de ensayo

## Antes de tomar datos

1. Punzón por lo menos 1 cm por encima de la cúpula y nada tocando las celdas.
2. Shield: puente V en 3V3 y conector DC sin conectar.
3. Si cambiaste el código: subir `fsr_ensayo` (Upload Speed 115200, sin los 12 V).
4. Cerrar el Monitor Serie y el Serial Plotter de Arduino IDE.
5. Conectar el USB del ESP32.
6. Conectar la fuente de 12 V (siempre después del USB).
7. Página → Ensayo → ESP32 por USB → Conectar.
8. `s`: revisar «límites 40 N y 2000 mV», Rm 10 kΩ y modo suma (`vl 2000` si dice 2500: con 2500 este FSR no llega al límite y el motor empuja de más).
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
2. Escalones finos hasta el límite del FSR: `p 0.05 99 3 0.2` (0,05 mm por escalón, pausa de 3 s, hasta `vl`).
3. En el análisis, la fuerza mínima es la de las celdas en el primer escalón donde el voltaje del FSR supera la línea base más el umbral de ruido (lo que reporta `fsr_lectura`).

### Retardo del FSR y rigidez de la silicona

Con un solo ensayo se miden las dos cosas. La idea: las celdas notan el primer toque del punzón mucho antes que el FSR.

- **Base:** los primeros 0,25 mm de cada bajada, con el punzón en el aire. Se usa la mediana y una desviación robusta, y solo las lecturas nuevas del HX711 (el firmware manda 50 líneas/s pero las celdas cambian 10 veces/s). Si la fuerza o el voltaje cambian dentro de la base, el ciclo se marca: el punzón ya tocaba o el FSR no estaba en reposo.
- **h₁ (contacto):** la primera lectura nueva de las celdas que pasa la base + máx(6 σ, umbral mínimo), confirmada por las dos siguientes. Cada fuerza se ubica donde estaba el punzón **250 ms antes**, porque el filtro del HX711 a 10 muestras/s tarda ~400 ms en asentarse. Se afina con la forma de Hertz (F^(2/3) crece lineal con z).
- **h₂ (el FSR responde):** donde el voltaje del FSR sube **ΔV** por encima de su base durante N líneas seguidas. Se busca desde el final de la base, **no desde h₁**: si el FSR responde antes que las celdas, Δh sale negativo y se avisa. ΔV, N, el umbral mínimo de las celdas y el retraso se eligen en Análisis; la fila «Δh según el umbral» muestra Δh para ΔV de 10 a 1500 mV.
- **Revisión mecánica:** `z` es la cuenta de pasos del motor, no una medición. Si la fuerza deja de subir mientras z sigue bajando, o si al subir la fuerza se suelta mucho más abajo de donde apareció al bajar, el punzón no se movió lo que dice z (algo cede, un tope o pasos perdidos); la página lo marca y no usa esa parte para la rigidez.
- **Piso del ADC:** el ESP32 no lee por debajo de ~150 mV (marca ~142 mV). El FSR sin carga está por debajo de eso; «responde» cuando su resistencia baja de ~200 kΩ con Rm = 10 kΩ.
- **Ciclos usados:** cada ciclo tiene una columna «Revisión». Los que tienen problemas no entran en el promedio (se pueden marcar a mano), y el resultado es la mediana.
- **Resolución:** Δh no puede medirse mejor que la distancia que baja el punzón entre dos lecturas de las celdas (velocidad / 10 por segundo): 0,05 mm a 0,5 mm/s, 0,005 mm a 0,05 mm/s.
- **Retardo:** Δh = h₂ − h₁ en mm, en vueltas de la varilla (Δh / 1,25 mm), en tiempo (Δt) y la fuerza de las celdas en h₂ (la fuerza mínima que detecta el FSR en este montaje).
- **Rigidez:** con la indentación d = z − h₁ − C·F se ajusta Hertz F = (4/3) · E/(1 − ν²) · √R · d^1,5 hasta d = 0,15 R. Sale el módulo E, la rigidez tangente dF/dd y la energía que disipa cada ciclo (área entre carga y descarga).

Pasos:

1. **Una sola vez, flexibilidad del montaje (C):** pon un bloque rígido (aluminio, acero) en lugar de la cúpula, `h`, y el ensayo **Contacto y rigidez** con 1 mm de profundidad y límite de fuerza bajo (`f 20`). En Análisis, **Usar este ensayo como montaje rígido**: C = pendiente de z contra F. Queda guardado para los siguientes análisis. Sin esto, la silicona parece más blanda de lo que es.
2. Monta la cúpula y haz `t` (tara) sin contacto.
3. `vl 2000`: el motor empieza a subir apenas el FSR responde bien. Con `vl 2500` este FSR no llega al límite (su voltaje se aplana cerca de 2450 mV) y el motor sigue empujando: ahí aparece el ruido fuerte y el movimiento perdido.
4. `h` (contacto), `m -3` (sube 3 mm más) y `z` (z = 0 ahí), esperando a que termine cada uno. Así el ensayo empieza con el punzón en el aire.
5. **Por escalones** (recomendado): `p 0.05 99 1 0.2`. Baja de a 0,05 mm y se queda quieto 1 s en cada escalón, hasta 4,95 mm o el límite del FSR, y vuelve igual (~4 min). En cada escalón la página promedia la fuerza y el FSR después de 0,45 s quieto: el retraso del HX711 (~0,2 s) no afecta y Δh sale con ± 0,05 mm (un escalón). Con ciclos continuos (`r`) a 0,5 mm/s, ese retraso equivale a 0,1 mm y la fuerza en h₂ queda muy incierta.
6. **Comprobación con papel** (opcional): con el motor quieto, una tira de papel sobre la cúpula; baja con −0,05 hasta que quede prensada y anota z (toque real, sin depender de las celdas). Sigue bajando hasta que suba el FSR y anota z otra vez.
6b. Para los ensayos normales, vuelve a hacer `h`.
7. En **Análisis**: pon el radio R de la cúpula y ν (0,49 para silicona). La tarjeta **Contacto y respuesta del FSR** muestra h₁, h₂, Δh, vueltas, Δt, F al responder y E por ciclo; los indicadores de arriba dan la media ± desviación.
8. **Llevar el retardo al modelo virtual** copia la media de Δh al parámetro «Retardo» del modelo (pestaña Modelo). En el modelo, hasta esa indentación el FSR no recibe fuerza y después recibe solo la que se suma desde ahí.

La resolución en z es de 1,25 mm / 3200 micropasos = 0,0004 mm; lo que limita es la frecuencia de las celdas y la velocidad, por eso el ensayo es lento.

### Ensayos sugeridos

| Ensayo | Comando | Para qué |
|---|---|---|
| Contacto y rigidez | `r 8 0.05 2 3` | Retardo del FSR (h₁ → h₂), fuerza mínima, módulo E de la silicona |
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
- **Retardo del FSR:** Δh = h₂ − h₁ (mm y vueltas) y la fuerza en h₂, media ± desviación de los ciclos.
- **Rigidez de la silicona:** módulo E (Hertz), rigidez tangente y energía disipada por ciclo, con la flexibilidad del montaje C indicada.

La pestaña **Análisis** de la página calcula todo esto a partir del CSV (`analisis/analizar_ensayo.py` calcula las curvas del sensor, sin el retardo ni la rigidez).
