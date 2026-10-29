# Montaje · prensa del FSR

![Prensa](../fotos/prensa.jpg)

La prensa tiene tres partes que se mueven o miden:

1. **Tornillo y punzón:** el NEMA 17 (abajo) gira la varilla; la tuerca del brazo rosado convierte el giro en bajada. El pin de acero de arriba evita que el brazo gire.
2. **Plataforma gris:** sostiene la cúpula de silicona en el asiento de arcos. Va apoyada sobre las celdas de carga.
3. **Celdas de carga:** se doblan un poco con la fuerza y el HX711 lo convierte en un número.

## 1. Celdas de carga (barra de 10 kg)

- Se montan **en voladizo**: un extremo atornillado a la estructura y el otro extremo cargando la plataforma. Cada extremo con un **separador** (arandelas o un taco impreso de 2–3 mm) para que la barra solo toque en sus bloques de tornillos.
- La **zona central** (donde está el pegamento blanco de las galgas) debe quedar libre: nada la toca, ni cables ni la plataforma.
- La **flecha** de la etiqueta apunta en el sentido de la fuerza: **hacia abajo**.
- No aprietes los tornillos sujetando la barra por el centro: tuerce las galgas. Sujeta por el bloque que estás atornillando.
- Capacidad: 10 kg ≈ 98 N por celda. No presiones con la mano con todo tu peso: la sobrecarga permanente empieza alrededor del 150 %.

## 2. ¿Dónde va la segunda celda?

El firmware y la página aceptan las dos formas (comando `modo suma` o `modo punzon`, o el selector en la pestaña **Ensayo**):

| Modo | Cómo se monta | Qué mide |
|---|---|---|
| **suma** | Las dos celdas debajo de la plataforma, en paralelo, a la misma altura | F = F1 + F2. La suma no depende de dónde quede la cúpula sobre la plataforma |
| **punzón** | Celda 1 debajo de la plataforma; celda 2 entre el brazo rosado y el punzón | F = F1. La celda 2 da una segunda medida del mismo esfuerzo para validar |

Si las dos están debajo de la plataforma, verifica que la plataforma no quede "coja": las dos deben recibir carga con la plataforma sola (después de la tara, empuja con un dedo en el centro y mira que F1 y F2 suban parecido).

## 3. Cúpula y FSR

- El FSR402 va centrado bajo el ápice de la cúpula, con la cara activa (la del círculo con el patrón) hacia el punzón.
- Saca los dos cables por una ranura del asiento sin que queden pisados: un cable apretado precarga el sensor y cambia la curva.
- La cúpula debe quedar **centrada bajo el punzón**. Baja el punzón despacio (`v 0.5`, `m 1`) hasta casi tocar y mira de frente y de lado.

## 4. Punzón

- La cara del punzón debe quedar **paralela** a la plataforma. Prueba: tres tiras de papel bajo el punzón en tres lados; al bajar, las tres deben quedar atrapadas al mismo tiempo.
- Anota el **diámetro** del punzón y el **radio** de la cúpula: son parámetros de la pestaña **Modelo**.

## 5. Tornillo, tuerca y juego

- Mide el avance real: marca el brazo, manda `m 10` y mide. Con varilla **M8** el avance es **1,25 mm por vuelta**; con tornillo **T8**, **8 mm**. Pon el valor en `LEAD_MM` del firmware.
- La tuerca del brazo tiene juego (backlash). Los ensayos del firmware siempre llegan a la cúpula **desde arriba** y empiezan 0,5 mm por encima del contacto, así que el juego se elimina antes de tocar.
- El acople flexible 5×8 debe tener los prisioneros sobre las caras planas del eje del motor.
- Lubrica la varilla con grasa de litio: menos fricción = menos corriente y menos calor en el A4988.

## 6. Cables

- Cada HX711 lo más cerca posible de su celda; los 4 hilos de la celda trenzados.
- Los cables del motor lejos de los de las celdas y del FSR. Si se cruzan, que sea a 90°.
- El ESP32 y el A4988 en una placa aparte, fijos a la base.

## Lista de chequeo antes de energizar

- [ ] Celdas en voladizo, con separadores y la zona central libre
- [ ] Flechas de las celdas hacia abajo
- [ ] Cúpula centrada y cables del FSR sin pisar
- [ ] Punzón paralelo y al menos 5 mm por encima de la cúpula
- [ ] A4988: RST + SLP a 3,3 V, MS1–MS3 a 3,3 V, EN a GPIO27 (sin puente con MS1)
- [ ] C1 100 µF en VMOT con la raya hacia GND
- [ ] HX711 a 3,3 V (no a 5 V)
- [ ] GND del ESP32 unido al GND del adaptador de 12 V
- [ ] Vref ≈ 0,8 V antes de conectar el motor
