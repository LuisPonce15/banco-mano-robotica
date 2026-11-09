// ============================================================
//  Banco FSR · LECTURA DEL FSR402 (prueba de conexión)
//  Solo lee el divisor del FSR. No usa motor ni celdas ni librerías.
//
//  Circuito:  3V3 ─ FSR402 ─┬─ GPIO34
//                           └─ Rm (10 kΩ) ─ GND
//
//  Al arrancar mide 2 s sin tocar el sensor (línea base) y calcula el
//  umbral de detección: el voltaje mínimo que se distingue del ruido.
//
//  Ver los datos:
//    · Herramientas → Serial Plotter (115200): gráfica en vivo   (modo p)
//    · Herramientas → Serial Monitor  (115200): texto legible     (modo t)
//    · Página web, pestaña Ensayo → ESP32 por USB                 (modo w)
//
//  Comandos (Monitor Serie, "Nueva línea"):
//    p / t / w    modo plotter / texto / página web
//    b            repetir la línea base (sin tocar el sensor)
//    r 10000      cambiar Rm (ohmios) si usas otra resistencia
//    ?            ayuda
// ============================================================
#include <Arduino.h>

const int      FSR_PIN   = 34;      // ADC1: funciona aunque el WiFi esté activo
const float    VCC_MV    = 3300.0;  // voltaje del divisor
const int      N_AVG     = 16;      // lecturas promediadas por muestra
const uint32_t PERIOD_MS = 20;      // 50 muestras por segundo

float rmOhm = 10000.0;              // resistencia fija del divisor
char  mode  = 'p';                  // 'p' plotter, 't' texto, 'w' web
float baseMv = 0, noiseMv = 0, thrMv = 30;
bool  detecting = false;
float peakMv = 0;

float readMv() {
  uint32_t acc = 0;
  for (int i = 0; i < N_AVG; i++) acc += analogReadMilliVolts(FSR_PIN);
  return acc / (float)N_AVG;
}

// Resistencia del FSR a partir del voltaje: R = Rm·(Vcc − V)/V
float fsrOhm(float mv) {
  if (mv < 1.0f) return INFINITY;
  return rmOhm * (VCC_MV - mv) / mv;
}

void say(const String& s) { Serial.print("# "); Serial.println(s); }

void baseline() {
  say("Línea base: NO toques el sensor durante 2 s...");
  delay(300);
  const int n = 100;
  double s = 0, s2 = 0; float mx = 0;
  for (int i = 0; i < n; i++) {
    float v = readMv(); s += v; s2 += v * v; if (v > mx) mx = v;
    delay(PERIOD_MS);
  }
  baseMv  = s / n;
  noiseMv = sqrt(max(0.0, s2 / n - baseMv * baseMv));
  // umbral: base + 6 desviaciones + 5 mV de margen, y al menos 20 mV por encima del máximo visto
  thrMv = max(baseMv + 6 * noiseMv + 5, mx + 20);
  say(String("Base = ") + String(baseMv, 1) + " mV · ruido = " + String(noiseMv, 2) + " mV · umbral = " + String(thrMv, 1) + " mV");
  if (baseMv > 300) say("La base está alta: ¿el sensor está presionado o la cúpula lo precarga? ¿FSR y Rm están al revés?");
  float rThr = fsrOhm(thrMv);
  say(String("El sensor 'detecta' cuando R_FSR baja de ") + String(rThr / 1000.0, 0) + " kΩ (con Rm = " + String(rmOhm, 0) + " Ω)");
}

void help() {
  say("=== Banco FSR · lectura del FSR402 (GPIO34) ===");
  say("p  modo Serial Plotter   t  modo texto   w  modo página web");
  say("b  repetir línea base    r 10000  cambiar Rm (Ω)   ?  ayuda");
}

void handle(String line) {
  line.trim();
  if (!line.length()) return;
  char c = tolower(line[0]);
  if (c == 'p' || c == 't' || c == 'w') { mode = c; say(c == 'p' ? "Modo Serial Plotter" : c == 't' ? "Modo texto" : "Modo página web (líneas D,...)"); }
  else if (c == 'b') baseline();
  else if (c == 'r') { float v = line.substring(1).toFloat(); if (v > 0) rmOhm = v; say(String("Rm = ") + String(rmOhm, 0) + " Ω"); baseline(); }
  else if (c == '?') help();
  else say("No entendí. Escribe ? para ver los comandos.");
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(FSR_PIN, INPUT);
  delay(500);
  help();
  baseline();
}

void loop() {
  if (Serial.available()) handle(Serial.readStringUntil('\n'));

  static uint32_t t0 = 0, tText = 0;
  if (millis() - t0 < PERIOD_MS) return;
  t0 = millis();

  float mv = readMv();
  float r  = fsrOhm(mv);

  // detección con histéresis para que no parpadee justo en el umbral
  bool was = detecting;
  if (!detecting && mv > thrMv) { detecting = true; peakMv = mv; }
  else if (detecting && mv < baseMv + 0.6f * (thrMv - baseMv)) detecting = false;
  if (detecting && mv > peakMv) peakMv = mv;

  if (mode == 'p') {
    // Serial Plotter: etiqueta:valor
    Serial.printf("V_mV:%.1f,Umbral_mV:%.1f,Detecta:%d\n", mv, thrMv, detecting ? 3300 : 0);
  } else if (mode == 'w') {
    // mismo formato que fsr_ensayo para la página: D,t_ms,z,F1,F2,Vfsr_mV,fase,ciclo
    Serial.printf("D,%lu,0.0000,0.0000,0.0000,%.1f,0,0\n", (unsigned long)millis(), mv);
  } else if (millis() - tText >= 250) {
    tText = millis();
    String rs = isinf(r) || r > 5e6 ? String("abierto") : r >= 1000 ? String(r / 1000.0, 1) + " kΩ" : String(r, 0) + " Ω";
    Serial.printf("V = %7.1f mV | R_FSR = %-10s | G = %8.2f µS | %s\n",
                  mv, rs.c_str(), isinf(r) ? 0.0 : 1e6 / r, detecting ? "DETECTA" : "-");
  }

  if (detecting != was && mode != 'p') {
    if (detecting) say(String("Empieza a detectar: ") + String(mv, 1) + " mV");
    else           say(String("Deja de detectar. Pico: ") + String(peakMv, 1) + " mV · R mín ≈ " + String(fsrOhm(peakMv) / 1000.0, 2) + " kΩ");
  }
}
