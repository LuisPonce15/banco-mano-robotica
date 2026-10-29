# Bitácora de depuración

Fallas encontradas durante la puesta en marcha y cómo se resolvieron. La pestaña **Depuración** de la página tiene un asistente que hace este diagnóstico a partir de tus mediciones.

## 1. Error de compilación "stray '\314' in program"

**Causa:** se pegó la ruta del archivo dentro de un sketch vacío en vez de abrir el archivo.
**Solución:** abrir `banco_prueba.ino` con *Archivo → Abrir* (el archivo tiene que estar en una carpeta con el mismo nombre).

## 2. Puerto equivocado

En Mac aparece `/dev/cu.wlan-debug`, que no es el ESP32. El correcto es **`/dev/cu.usbserial-0001`**. Si no aparece, falta el driver CP210x o el cable USB es solo de carga.

## 3. "'ledcAttach' was not declared in this scope"

**Causa:** paquete esp32 2.x; `ledcAttach` es de la versión 3.x.
**Solución:** el código usa `#if ESP_ARDUINO_VERSION_MAJOR >= 3` para funcionar con las dos versiones. Igual se recomienda actualizar a 3.x.

## 4. "No serial data received" al subir

Se probaron el botón BOOT, otra velocidad y otro cable. **Solución:** cambiar el ESP32 (el primero estaba dañado).

## 5. Solo aparece un INA219 en el escaneo I²C

**Causa:** los dos módulos tienen los mismos puentes A0/A1, así que tienen la misma dirección (0x44 en el segundo par de módulos).
**Solución:** ver [ina219.md](ina219.md#dirección-ic). *Pendiente.*

## 6. Los motores no se bloquean al habilitar (`e`)

Mediciones en el driver A:

| Punto | Medido | Esperado | Veredicto |
|---|---|---|---|
| VMOT | 12 V | 8–35 V | ✔ |
| VDD | 3,3 V | 3,0–5,5 V | ✔ |
| Vref | 0,76 V | ≈ 0,8 V | ✔ (≈ 0,95 A) |
| SLP / RST (antes) | 1,01 V | > 2,3 V | ✘ flotando |
| SLP / RST (después de unirlos a VDD) | 3,31 V | > 2,3 V | ✔ |
| GPIO27 en el ESP32, con `e` / `d` | 10 mV / 3,3 V | < 0,3 V / > 3 V | ✔ el ESP32 sí cambia |
| EN en el driver, con `e` / `d` | 2,73 V / 2,70 V | < 1 V / > 2,3 V | ✘ no sigue a GPIO27 |

**Conclusión:** el ESP32 hace bien su trabajo, pero el pin EN del driver no recibe esa señal. Dos causas posibles:

1. **Cable D27 → EN abierto** (dupont suelto, soldadura fría o pin equivocado).
2. **EN unido a 3,3 V** por un puente de soldadura con **MS1**, que está justo al lado.

Que EN marque 2,7 V y no 3,3 V con GPIO27 en 0 V apunta a que el cable está abierto y EN está flotando o lo levanta algo con resistencia.

**Pruebas (con todo apagado):**

- Continuidad entre el pin D27 del ESP32 y el pin EN del driver: debe pitar.
- Resistencia entre EN y MS1: debe dar **OL** (abierto). Si da ~0 Ω, hay un puente de soldadura.
- Prueba rápida: desconecta EN del ESP32 y únelo a GND con un cable. Si el motor queda con fuerza al encender, el driver está bien y el problema es el cable.

*Pendiente de confirmar.*
