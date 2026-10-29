# Banco E-01 · banco de pruebas para mano robótica

[![Firmware](https://github.com/LuisPonce15/banco-mano-robotica/actions/workflows/firmware.yml/badge.svg)](https://github.com/LuisPonce15/banco-mano-robotica/actions/workflows/firmware.yml) [![Sitio](https://github.com/LuisPonce15/banco-mano-robotica/actions/workflows/sitio.yml/badge.svg)](https://github.com/LuisPonce15/banco-mano-robotica/actions/workflows/sitio.yml)

Banco de pruebas de dos ejes lineales sincronizados (NEMA 17 + varilla roscada T8) para caracterizar un dedo robótico de 2 GDL accionado por tendón. Un servo **DS3240** flexiona el dedo y un servo **DS3235** lo desvía lateralmente. Un **INA219** mide la corriente de cada servo. Todo se controla con un **ESP32** y se alimenta de la pared (110 V) con una fuente de 12 V.

Proyecto de Desarrollo Profesional · Universidad de los Andes · Luis Felipe Ponce.

**▶ Página interactiva: https://luisponce15.github.io/banco-mano-robotica/**

El repositorio tiene dos bancos independientes, cada uno con su propio ESP32:

| Banco | Qué hace | Página | Carpeta |
|---|---|---|---|
| **E-01 · dedo** | Dos ejes NEMA 17 sincronizados, servos DS3240/DS3235 e INA219 | [abrir](https://luisponce15.github.io/banco-mano-robotica/) | `firmware/`, `hardware/` |
| **FSR · prensa** | Caracteriza un FSR402 dentro de silicona con una prensa de tornillo y 2 celdas de carga (HX711) | [abrir](https://luisponce15.github.io/banco-mano-robotica/fsr/) | [`fsr/`](fsr/README.md) |

La página del banco E-01 reúne:


| Pestaña | Qué tiene |
|---|---|
| **Esquema** | Esquemático global interactivo: pasa el cursor por cada cable o componente para ver su corriente en vivo. |
| **Simulación** | Los dos ejes sincronizados (con pérdida de pasos), los servos, el dedo en 2 vistas y las corrientes que leería el INA219. Incluye gráficas congelables y exportables a PNG y CSV. |
| **Conexiones** | Plano E-01 completo: entrada de 110 V, potencia, A4988, INA219, Wago, lista de cables, pines, protecciones y energización por etapas. |
| **Sistema y modelo** | Parámetros físicos editables y las ecuaciones que usa la simulación. |
| **Código** | Todo el código del repositorio con resaltado de sintaxis, índice de funciones, búsqueda y botones de copiar y descargar. |
| **Puesta en marcha** | Guía paso a paso, de cero a la Fase 1, con lista de chequeo que se guarda en el navegador. |
| **Depuración** | Asistente de diagnóstico del A4988 a partir de tus mediciones, calculadora de direcciones INA219, Vref, pasos/mm y pulsos de servo. |
| **Consola USB** | Maneja el ESP32 desde el navegador por USB (Web Serial, en Chrome o Edge): botones para cada comando de `banco_prueba`. Tiene un **modo demo** que emula el firmware sin hardware. |

## Estructura

```
banco-mano-robotica/
├── firmware/
│   ├── banco_prueba/       Prueba de hardware por Monitor Serie (sin librerías extra)
│   └── banco_fase1/        Fase 1: ejes M1 + M2 por WiFi con homing (página servida por el ESP32)
├── hardware/
│   ├── BOM.md              Lista de componentes
│   ├── pinout.md           Pines del ESP32 y del A4988
│   ├── cableado.md         Cadena de potencia, mapa de Wago, motores, INA219
│   ├── cables.csv          Los 43 cables con calibre, color y terminal
│   ├── a4988.md            Conexión, Vref, microstepping y calor
│   ├── ina219.md           Direcciones I2C, rango del shunt y código
│   ├── seguridad.md        Orden de encendido/apagado, USB y energización por etapas
│   └── depuracion.md       Bitácora de fallas encontradas y cómo se resolvieron
├── fsr/                    Banco FSR: firmware, conexiones, montaje, calibración, análisis y fotos
├── web/index.src.html      Fuente de la página del banco E-01
├── web/fsr.src.html        Fuente de la página del banco FSR
├── web/comun/repo.js       Visor de código y Markdown compartido
├── tools/build_site.py     Genera docs/ e incrusta el código del repo en cada página
├── docs/                   Sitio publicado en GitHub Pages (generado, no editar a mano)
└── .github/workflows/      CI: compila el firmware y revisa que el sitio esté al día
```

## Inicio rápido

1. Instala **Arduino IDE 2** y el paquete **esp32 by Espressif 3.x** (Herramientas → Placa → Gestor de placas).
2. Placa: **ESP32 Dev Module**. Puerto: `/dev/cu.usbserial-0001` en Mac o `COMx` en Windows.
3. Abre `firmware/banco_prueba/banco_prueba.ino`, súbelo y abre el Monitor Serie a **115200** con fin de línea **Nueva línea**.
4. Escribe `?` para ver los comandos. O usa la pestaña **Consola USB** de la página, que manda los comandos con botones.

> ⚠️ **Antes de conectar el USB** desconecta el cable **C40** (5 V del buck U2 → VIN del ESP32). Para apagar: primero **S2** en OFF, después **S1** o desenchufar, y el USB al final. Detalles en [`hardware/seguridad.md`](hardware/seguridad.md).

## Hardware en resumen

| Bloque | Componente |
|---|---|
| Entrada AC | Módulo IEC C14 con fusible **F1 T4 A** e interruptor **S1** |
| Fuente | Conmutada **12 V · 10 A** (selector en 110 V, V.ADJ en 12,0 V) |
| Corte de 12 V | Switch de palanca **S2** (10 A 125 VAC) entre +V y W1 |
| Ejes | 2 × NEMA 17 + varilla T8 (avance 8 mm), 2 × **A4988** a 1/16 de paso |
| Servos | **DS3240** (flexión, adelante) y **DS3235** (lateral, atrás) a 6 V |
| Reductores | U1 buck 12→6 V (XL4016) para servos · U2 LM2596 12→5 V para el ESP32 |
| Medición | 2 × **INA219** en serie con el + de cada servo (I²C en GPIO21/22) |
| Distribución | 5 × Wago 221-415 (W1 +12 V, W2 GND, W3 +6 V, W4 GND servos, W5 GND lógica) |
| Paro | **BTN** arcade NC en GPIO23 (paro por software) + **S2** (corte físico) |

## Estado

- [x] Montaje mecánico y eléctrico
- [x] `banco_prueba` compila y corre en el ESP32
- [x] `banco_fase1` compila (paquete esp32 3.x) en la integración continua
- [x] Finales de carrera FC-A, FC-B y botón BTN verificados
- [x] VMOT, VDD, Vref y SLP/RST de los drivers verificados
- [ ] Motores con fuerza: EN del driver no sigue a GPIO27 (ver [`hardware/depuracion.md`](hardware/depuracion.md))
- [ ] Dos INA219 con direcciones distintas
- [ ] Medir sentido de giro y mm por vuelta → ajustar `INVERT_A/B` y `LEAD_MM`
- [ ] Fase 1: ejes por WiFi con homing
- [ ] Fase 2: servos + lectura INA219 a 100 Hz
- [ ] Fase 3: gráficas reales en la página (registro y exportación)

## Regenerar la página

Las páginas publicadas (`docs/index.html` y `docs/fsr/index.html`) se generan a partir de `web/*.src.html` y de los archivos del repositorio, para que la pestaña **Código** siempre muestre la versión actual:

```bash
python3 tools/build_site.py
```

El flujo de CI falla si olvidas regenerarla después de cambiar código.

## Licencia

[MIT](LICENSE)
