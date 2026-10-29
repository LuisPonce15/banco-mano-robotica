# Firmware

Los dos programas son para **ESP32 DevKit V1** con el paquete **esp32 by Espressif**. `banco_prueba` funciona con 2.x y 3.x; `banco_fase1` necesita **3.x** (lo piden sus librerías). La compilación automática (pestaña *Actions*) revisa las dos combinaciones en cada cambio. Placa en Arduino IDE: **ESP32 Dev Module**.

| Sketch | Para qué | Librerías |
|---|---|---|
| [`banco_prueba`](banco_prueba/banco_prueba.ino) | Probar cada pieza del hardware por separado desde el Monitor Serie | ninguna |
| [`banco_fase1`](banco_fase1/banco_fase1.ino) | Mover los dos ejes sincronizados por WiFi, con homing y paro | FastAccelStepper, ESP Async WebServer, Async TCP, ArduinoJson 7 |

## Pines (iguales en los dos)

| GPIO | Función |
|---|---|
| 25 / 26 | STEP / DIR eje A (DRV-A) |
| 32 / 33 | STEP / DIR eje B (DRV-B) |
| 27 | EN de ambos A4988 (LOW = con fuerza) |
| 18 / 19 | Final de carrera A / B (NC a GND, `INPUT_PULLUP`: HIGH = tocado o cable roto) |
| 23 | BTN de paro por software (NC a GND) |
| 16 / 17 | Servo DS3240 / DS3235 (50 Hz, 500–2500 µs) |
| 21 / 22 | I²C SDA / SCL (INA219) |

## banco_prueba

Monitor Serie a **115200**, fin de línea **Nueva línea**. Comandos:

| Comando | Acción |
|---|---|
| `?` | ayuda |
| `s` | estado de FC-A, FC-B, BTN y motores |
| `m` | monitor en vivo de entradas (cualquier tecla para salir) |
| `i` | buscar dispositivos I²C (INA219) |
| `e` / `d` | habilitar / soltar motores (EN) |
| `a 800` / `b -800` | mover el eje A o B N pasos (negativo = al revés) |
| `v 800` | velocidad en pasos/s (50 a 4000) |
| `1 90` / `2 90` | servo DS3240 / DS3235 al ángulo indicado (0–180°) |
| `x` | quitar la señal de los servos |

Cualquier tecla durante un movimiento lo detiene. El movimiento también se detiene si se presiona BTN o se toca el final de carrera del eje.

Con 1/16 de paso y varilla de 8 mm: **3200 pasos = 1 vuelta = 8 mm**, es decir, **400 pasos/mm**.

## banco_fase1

1. Instala las librerías desde *Herramientas → Administrar bibliotecas*: **FastAccelStepper** (gin66), **ESP Async WebServer** y **Async TCP** (ESP32Async), **ArduinoJson** (Benoît Blanchon, versión 7).
2. Revisa los ajustes al inicio del archivo: `LEAD_MM`, `MICROSTEP`, `STROKE_MM`, `INVERT_A`, `INVERT_B`.
3. Súbelo. El ESP32 crea la red WiFi **BancoE01** (clave `mano2026`).
4. Conéctate a esa red y abre **http://192.168.4.1**.
5. Habilita los motores, haz **Homing** y mueve los ejes con el deslizador.

La página de control vive dentro del ESP32. La página de GitHub Pages no se puede comunicar con él por WiFi (el navegador bloquea pasar de `https` a `ws://` local), pero sí por USB desde la pestaña **Consola USB**.
