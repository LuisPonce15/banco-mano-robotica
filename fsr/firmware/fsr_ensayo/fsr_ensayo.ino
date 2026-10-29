// ============================================================
//  Banco FSR · caracterización de un FSR402 dentro de silicona
//  ESP32 DevKit V1 + A4988 + NEMA 17 (17HS4401S) + 2× HX711 + 2 celdas de 10 kg
//
//  El punzón (brazo rosado) baja con el tornillo y presiona la cúpula de
//  silicona que tiene el FSR adentro. Las celdas de carga miden la fuerza
//  real y el ESP32 lee el voltaje del divisor del FSR.
//
//  Se maneja desde el Monitor Serie (115200, "Nueva línea") o desde la
//  pestaña "Ensayo" de la página web (Chrome o Edge, por USB).
//
//  Comandos:
//    ?                  ayuda
//    s                  estado
//    e / d              motor con fuerza / suelto
//    t                  tara de las dos celdas (sin el punzón tocando)
//    c1 500 / c2 500    calibrar la celda 1 o 2 con una masa conocida (g)
//    v 1                velocidad de los movimientos manuales (mm/s)
//    m 2.5              mover (mm). + baja hacia la muestra, − sube
//    z                  definir la posición actual como z = 0
//    h                  buscar el contacto: baja lento hasta sentir fuerza (z = 0)
//    r 3 0.5 2 3        ciclos: profundidad 3 mm, a 0,5 mm/s, pausa 2 s, 3 ciclos
//    p 0.5 6 5 0.5      escalones: 0,5 mm por escalón, 6 escalones, 5 s cada uno, a 0,5 mm/s
//    f 40               límite de fuerza (N). Si se supera, se detiene y sube
//    o 50               datos por segundo (0 = sin datos)
//    rm 10000           resistencia fija del divisor del FSR (ohmios)
//    modo suma / modo punzon   cómo se combinan las celdas
//    x                  detener todo
//
//  Líneas que manda el ESP32:
//    # texto            mensajes para leer
//    D,t_ms,z_mm,F1_N,F2_N,Vfsr_mV,fase,ciclo   datos del ensayo
//    S,...              estado (ver printStatus)
// ============================================================
#include <Arduino.h>
#include <Preferences.h>
#include <FastAccelStepper.h>

// ---------------- AJUSTES (revísalos antes de cargar) ----------------
const float LEAD_MM       = 1.25;   // avance del tornillo por vuelta: varilla M8 = 1,25 mm; T8 = 8 mm. Mídelo.
const int   MICROSTEP     = 16;     // MS1, MS2 y MS3 a 3,3 V
const bool  INVERT_DIR    = false;  // true si con "m 1" el punzón SUBE en vez de bajar
const float MAX_DEPTH_MM  = 20.0;   // nunca baja más que esto por debajo del contacto
const float Z_FREE_MM     = -0.5;   // posición de reposo: 0,5 mm por encima del contacto
const float F_CONTACT_N   = 0.05;   // fuerza que cuenta como contacto al buscarlo
const float CELL_RATED_N  = 98.07;  // celda de 10 kg
const float VCC_MV        = 3300.0; // alimentación del divisor del FSR
const float ACCEL_MM_S2   = 5.0;

// ---------------- PINES ----------------
const int STEP_PIN = 25, DIR_PIN = 26, EN_PIN = 27;  // A4988
const int HX1_DT = 16, HX1_SCK = 17;                  // HX711 de la celda 1
const int HX2_DT = 18, HX2_SCK = 19;                  // HX711 de la celda 2
const int FSR_PIN = 34;                               // nodo del divisor (ADC1)

const float STEPS_PER_MM = 200.0f * MICROSTEP / LEAD_MM;
const float G = 9.80665f;

// ---------------- HX711 ----------------
portMUX_TYPE hxMux = portMUX_INITIALIZER_UNLOCKED;

struct HX711 {
  int dt = -1, sck = -1;
  long raw = 0, offset = 0;
  float k = 21898.0f;          // cuentas por newton (10 kg, 1 mV/V, ganancia 128). Se corrige con c1/c2.
  uint32_t tLast = 0;
  bool seen = false, warned = false;

  void begin(int d, int s) {
    dt = d; sck = s;
    pinMode(dt, INPUT);
    pinMode(sck, OUTPUT);
    digitalWrite(sck, LOW);
  }
  bool ready() { return digitalRead(dt) == LOW; }
  // Lee 24 bits y deja el canal A con ganancia 128 (25 pulsos en total).
  bool read() {
    if (!ready()) return false;
    uint32_t v = 0;
    portENTER_CRITICAL(&hxMux);              // SCK en alto más de 60 µs apaga el HX711
    for (int i = 0; i < 24; i++) {
      digitalWrite(sck, HIGH); delayMicroseconds(1);
      v = (v << 1) | (uint32_t)digitalRead(dt);
      digitalWrite(sck, LOW);  delayMicroseconds(1);
    }
    digitalWrite(sck, HIGH); delayMicroseconds(1);
    digitalWrite(sck, LOW);  delayMicroseconds(1);
    portEXIT_CRITICAL(&hxMux);
    if (v & 0x800000) v |= 0xFF000000;       // signo de 24 bits
    raw = (int32_t)v;
    tLast = millis(); seen = true; warned = false;
    return true;
  }
  float newtons() const { return (raw - offset) / k; }
};

HX711 hx1, hx2;

// ---------------- Estado ----------------
FastAccelStepperEngine engine;
FastAccelStepper* st = nullptr;
Preferences prefs;

bool  enabled = false, homed = false, sumMode = true;
float vManual = 1.0f;          // mm/s
float fLimit  = 40.0f;         // N
float rmOhm   = 10000.0f;
int   streamHz = 50;
float lastMv  = 0;

enum Mode { IDLE, MANUAL, SEEK, TEST };
Mode mode = IDLE;

struct Seg { bool hold; float z, v, secs; uint8_t phase; uint16_t cycle; };
const int MAXSEG = 160;
Seg prog[MAXSEG];
int nSeg = 0, iSeg = -1;
bool segStarted = false;
uint32_t segT0 = 0;
uint8_t phase = 0;
uint16_t cycleNo = 0;

// ---------------- Utilidades ----------------
float zNow()           { return st->getCurrentPosition() / STEPS_PER_MM; }
long  mm2st(float mm)  { return lroundf(mm * STEPS_PER_MM); }
float fTotal()         { return sumMode ? hx1.newtons() + hx2.newtons() : hx1.newtons(); }

void say(const String& s) { Serial.print("# "); Serial.println(s); }

void setEnable(bool on) {
  enabled = on;
  digitalWrite(EN_PIN, on ? LOW : HIGH);
}

void setSpeed(float v) {
  v = constrain(v, 0.01f, 10.0f);
  st->setSpeedInHz((uint32_t)max(1.0f, v * STEPS_PER_MM));
  st->setAcceleration((int32_t)max(1.0f, ACCEL_MM_S2 * STEPS_PER_MM));
}

void stopAll(const char* why) {
  st->forceStop();
  mode = IDLE; nSeg = 0; iSeg = -1; phase = 0;
  if (why) say(why);
}

float readFsrMv() {
  uint32_t acc = 0;
  for (int i = 0; i < 8; i++) acc += analogReadMilliVolts(FSR_PIN);
  return acc / 8.0f;
}

// Promedia n lecturas de un HX711 (bloquea hasta ~n/10 s a 10 muestras/s)
bool average(HX711& h, int n, double& out) {
  double acc = 0; int got = 0; uint32_t t0 = millis();
  while (got < n && millis() - t0 < (uint32_t)(n * 150 + 500)) {
    if (h.read()) { acc += h.raw; got++; }
    delay(1);
  }
  if (got < n) return false;
  out = acc / got;
  return true;
}

void tare() {
  double a1, a2;
  bool ok1 = average(hx1, 16, a1), ok2 = average(hx2, 16, a2);
  if (ok1) hx1.offset = lround(a1);
  if (ok2) hx2.offset = lround(a2);
  say(String("Tara lista. Celda 1: ") + (ok1 ? "ok" : "SIN RESPUESTA") + " · Celda 2: " + (ok2 ? "ok" : "SIN RESPUESTA"));
}

void calibrate(int n, float grams) {
  HX711& h = (n == 1) ? hx1 : hx2;
  if (grams <= 0) { say("Escribe la masa en gramos, por ejemplo: c1 500"); return; }
  double a;
  if (!average(h, 24, a)) { say("La celda no responde: revisa el HX711."); return; }
  double d = a - h.offset;
  if (fabs(d) < 500) { say("Casi no veo carga. ¿Pusiste la masa? ¿Hiciste la tara sin ella?"); return; }
  h.k = d / (grams / 1000.0 * G);
  prefs.putFloat(n == 1 ? "k1" : "k2", h.k);
  say(String("Celda ") + n + " calibrada: " + String(h.k, 1) + " cuentas/N (guardado)."
      + (h.k < 0 ? " El signo es negativo: la celda está al revés, pero ya queda corregido." : ""));
}

// ---------------- Programa de ensayo ----------------
void addMove(float z, float v, uint8_t ph, uint16_t c) { if (nSeg < MAXSEG) prog[nSeg++] = {false, z, v, 0, ph, c}; }
void addHold(float s, uint8_t ph, uint16_t c)          { if (nSeg < MAXSEG) prog[nSeg++] = {true, 0, 0, s, ph, c}; }

bool canTest() {
  if (!enabled) { say("Primero habilita el motor con e."); return false; }
  if (!homed)   { say("Primero busca el contacto con h (o define z = 0 con z)."); return false; }
  if (mode != IDLE) { say("Hay un movimiento en curso. Detén con x."); return false; }
  return true;
}

void startProgram(const char* name) {
  iSeg = 0; segStarted = false; mode = TEST;
  say(String("INICIO ") + name + " (" + nSeg + " tramos)");
}

void cmdCycles(float depth, float v, float pause, int cycles) {
  if (!canTest()) return;
  if (depth <= 0 || depth > MAX_DEPTH_MM) { say(String("Profundidad entre 0 y ") + MAX_DEPTH_MM + " mm."); return; }
  v = constrain(v, 0.01f, 5.0f); pause = constrain(pause, 0.0f, 600.0f); cycles = constrain(cycles, 1, (MAXSEG - 2) / 4);
  nSeg = 0;
  addMove(Z_FREE_MM, 1.0f, 3, 0);
  for (int c = 1; c <= cycles; c++) {
    addMove(depth, v, 1, c);  addHold(pause, 2, c);
    addMove(Z_FREE_MM, v, 3, c); addHold(pause, 4, c);
  }
  startProgram("ciclos");
}

void cmdSteps(float step, int n, float pause, float v) {
  if (!canTest()) return;
  if (step <= 0 || n < 1 || step * n > MAX_DEPTH_MM) { say(String("Paso × escalones debe quedar entre 0 y ") + MAX_DEPTH_MM + " mm."); return; }
  v = constrain(v, 0.01f, 5.0f); pause = constrain(pause, 0.0f, 600.0f); n = min(n, (MAXSEG - 4) / 4);
  nSeg = 0;
  addMove(Z_FREE_MM, 1.0f, 3, 0);
  addMove(0, v, 1, 1);
  for (int i = 1; i <= n; i++)     { addMove(i * step, v, 1, 1); addHold(pause, 2, 1); }
  for (int i = n - 1; i >= 0; i--) { addMove(i * step, v, 3, 1); addHold(pause, 4, 1); }
  addMove(Z_FREE_MM, v, 3, 1);
  startProgram("escalones");
}

void runProgram() {
  if (mode != TEST) return;
  if (iSeg >= nSeg) { mode = IDLE; phase = 0; say("FIN del ensayo"); return; }
  Seg& s = prog[iSeg];
  phase = s.phase; cycleNo = s.cycle;
  if (!segStarted) {
    segStarted = true; segT0 = millis();
    if (!s.hold) { setSpeed(s.v); st->moveTo(mm2st(s.z)); }
    return;
  }
  bool done = s.hold ? (millis() - segT0 >= (uint32_t)(s.secs * 1000)) : !st->isRunning();
  if (done) { iSeg++; segStarted = false; }
}

// ---------------- Seguridad y contacto ----------------
void checkForce() {
  float F = fTotal(), f1 = hx1.newtons(), f2 = hx2.newtons();
  float worst = max(fabsf(f1), fabsf(f2));
  if (mode == SEEK && F >= F_CONTACT_N) {
    st->forceStop();
    while (st->isRunning()) delay(1);
    st->setCurrentPosition(0);
    homed = true; mode = IDLE;
    say("Contacto encontrado: z = 0. Subo 0,5 mm.");
    setSpeed(0.5f); st->moveTo(mm2st(Z_FREE_MM)); mode = MANUAL;
    return;
  }
  if (F > fLimit || worst > 0.95f * CELL_RATED_N) {
    bool goingDown = st->isRunning() && st->getPositionAfterCommandsCompleted() > st->getCurrentPosition();
    if (mode == TEST || goingDown) {
      stopAll(nullptr);
      say(String("ALTO: fuerza ") + String(F, 2) + " N supera el límite (" + String(fLimit, 1) + " N). Subo 1 mm.");
      setSpeed(1.0f); st->move(-mm2st(1.0f)); mode = MANUAL;
    }
  }
}

// ---------------- Mensajes ----------------
void printStatus() {
  const char* m = mode == IDLE ? "quieto" : mode == MANUAL ? "moviendo" : mode == SEEK ? "buscando_contacto" : "ensayo";
  Serial.printf("S,%d,%d,%s,%.4f,%.4f,%.4f,%.1f,%.1f,%.0f,%s,%.1f,%.1f,%.3f\n",
                enabled, homed, m, zNow(), hx1.newtons(), hx2.newtons(), lastMv, fLimit, rmOhm,
                sumMode ? "suma" : "punzon", hx1.k, hx2.k, LEAD_MM);
  say(String("Motor ") + (enabled ? "con fuerza" : "suelto") + " · " + m + " · z = " + String(zNow(), 3) + " mm"
      + (homed ? "" : " (sin contacto definido)"));
  say(String("F1 = ") + String(hx1.newtons(), 3) + " N · F2 = " + String(hx2.newtons(), 3) + " N · F = "
      + String(fTotal(), 3) + " N (" + (sumMode ? "suma de las dos" : "solo celda 1") + ")");
  say(String("FSR: ") + String(lastMv, 0) + " mV con Rm = " + String(rmOhm, 0) + " Ω · límite " + String(fLimit, 1) + " N");
}

void help() {
  const char* L[] = {
    "=== Banco FSR · caracterización ===",
    "?  ayuda            s  estado",
    "e / d               motor con fuerza / suelto",
    "t                   tara de las celdas (sin contacto)",
    "c1 500 / c2 500     calibrar celda 1 / 2 con 500 g encima",
    "v 1                 velocidad manual (mm/s)",
    "m 2.5               mover 2,5 mm (+ baja, - sube)",
    "z                   esta posición es z = 0",
    "h                   buscar el contacto con la silicona",
    "r 3 0.5 2 3         ciclos: prof. mm, mm/s, pausa s, ciclos",
    "p 0.5 6 5 0.5       escalones: paso mm, n, pausa s, mm/s",
    "f 40                límite de fuerza (N)",
    "o 50                datos por segundo (0 = apagado)",
    "rm 10000            resistencia del divisor del FSR",
    "modo suma | modo punzon",
    "x                   detener"
  };
  for (auto l : L) say(l);
}

// ---------------- Comandos ----------------
void handle(String line) {
  line.trim();
  if (!line.length()) return;
  int sp = line.indexOf(' ');
  String cmd = (sp < 0 ? line : line.substring(0, sp)); cmd.toLowerCase();
  String rest = sp < 0 ? "" : line.substring(sp + 1);
  float a[4] = {0, 0, 0, 0}; int na = 0;
  { String r = rest; r.trim();
    while (r.length() && na < 4) { int k = r.indexOf(' '); String tok = k < 0 ? r : r.substring(0, k);
      tok.replace(',', '.'); a[na++] = tok.toFloat(); r = k < 0 ? "" : r.substring(k + 1); r.trim(); } }

  if (cmd == "?") help();
  else if (cmd == "s") printStatus();
  else if (cmd == "e") { setEnable(true);  say("Motor con fuerza (EN = LOW)."); }
  else if (cmd == "d") { stopAll(nullptr); setEnable(false); say("Motor suelto (EN = HIGH)."); }
  else if (cmd == "x") { stopAll("Detenido."); }
  else if (cmd == "t") { if (mode != IDLE) say("Detén el movimiento antes de la tara."); else tare(); }
  else if (cmd == "c1" || cmd == "c2") calibrate(cmd == "c1" ? 1 : 2, a[0]);
  else if (cmd == "v") { vManual = constrain(a[0], 0.01f, 10.0f); say(String("Velocidad manual: ") + String(vManual, 2) + " mm/s"); }
  else if (cmd == "z") { if (mode != IDLE) say("Detén primero."); else { st->setCurrentPosition(0); homed = true; say("z = 0 aquí."); } }
  else if (cmd == "m") {
    if (!enabled) { say("Primero habilita el motor con e."); return; }
    if (mode == TEST || mode == SEEK) { say("Hay un ensayo en curso. Detén con x."); return; }
    float target = zNow() + a[0];
    if (homed && target > MAX_DEPTH_MM) { say(String("No bajo más de ") + MAX_DEPTH_MM + " mm bajo el contacto."); return; }
    setSpeed(vManual); st->moveTo(mm2st(target)); mode = MANUAL;
  }
  else if (cmd == "h") {
    if (!enabled) { say("Primero habilita el motor con e."); return; }
    if (mode != IDLE) { say("Detén primero con x."); return; }
    if (fTotal() > F_CONTACT_N) { say("Ya hay fuerza: sube el punzón (m -2) o repite la tara (t)."); return; }
    st->setCurrentPosition(0); homed = false;
    setSpeed(0.5f); st->moveTo(mm2st(40)); mode = SEEK;
    say("Buscando el contacto a 0,5 mm/s (máx. 40 mm)...");
  }
  else if (cmd == "r") cmdCycles(a[0], na > 1 ? a[1] : 0.5f, na > 2 ? a[2] : 2.0f, na > 3 ? (int)a[3] : 3);
  else if (cmd == "p") cmdSteps(a[0], na > 1 ? (int)a[1] : 5, na > 2 ? a[2] : 5.0f, na > 3 ? a[3] : 0.5f);
  else if (cmd == "f") { fLimit = constrain(a[0], 0.5f, 0.95f * CELL_RATED_N * (sumMode ? 2 : 1)); prefs.putFloat("flim", fLimit); say(String("Límite de fuerza: ") + String(fLimit, 1) + " N"); }
  else if (cmd == "o") { streamHz = constrain((int)a[0], 0, 100); say(String("Datos: ") + streamHz + " por segundo"); }
  else if (cmd == "rm") { if (a[0] > 0) { rmOhm = a[0]; prefs.putFloat("rm", rmOhm); } say(String("Rm = ") + String(rmOhm, 0) + " Ω"); }
  else if (cmd == "modo") {
    rest.toLowerCase();
    if (rest.startsWith("s")) sumMode = true; else if (rest.startsWith("p")) sumMode = false;
    prefs.putBool("suma", sumMode);
    say(sumMode ? "Modo suma: las dos celdas sostienen la plataforma, F = F1 + F2."
                : "Modo punzón: la celda 1 mide la fuerza (F = F1) y la celda 2 sirve de comparación.");
  }
  else say("No entendí. Escribe ? para ver los comandos.");
}

// ---------------- setup / loop ----------------
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(EN_PIN, OUTPUT);
  setEnable(false);
  hx1.begin(HX1_DT, HX1_SCK);
  hx2.begin(HX2_DT, HX2_SCK);
  pinMode(FSR_PIN, INPUT);

  prefs.begin("fsr", false);
  hx1.k   = prefs.getFloat("k1", hx1.k);
  hx2.k   = prefs.getFloat("k2", hx2.k);
  rmOhm   = prefs.getFloat("rm", rmOhm);
  fLimit  = prefs.getFloat("flim", fLimit);
  sumMode = prefs.getBool("suma", true);

  engine.init();
  st = engine.stepperConnectToPin(STEP_PIN);
  if (!st) { Serial.println("# Error: no pude configurar el pin STEP"); while (true) delay(1000); }
  st->setDirectionPin(DIR_PIN, !INVERT_DIR);
  setSpeed(vManual);

  delay(400);
  help();
  say("Haciendo tara inicial (deja la plataforma sin contacto)...");
  tare();
  printStatus();
}

void loop() {
  if (Serial.available()) handle(Serial.readStringUntil('\n'));

  bool n1 = hx1.read(), n2 = hx2.read();
  if (n1 || n2) checkForce();
  if (hx1.seen && millis() - hx1.tLast > 1500 && !hx1.warned) { hx1.warned = true; say("La celda 1 (HX711) dejó de responder."); }
  if (hx2.seen && millis() - hx2.tLast > 1500 && !hx2.warned) { hx2.warned = true; say("La celda 2 (HX711) dejó de responder."); }

  if (mode == MANUAL && !st->isRunning()) mode = IDLE;
  if (mode == SEEK && !st->isRunning()) { mode = IDLE; say("No encontré contacto en 40 mm. ¿La muestra está debajo? ¿INVERT_DIR?"); }
  runProgram();

  static uint32_t tData = 0;
  if (streamHz > 0 && millis() - tData >= 1000u / streamHz) {
    tData = millis();
    lastMv = readFsrMv();
    Serial.printf("D,%lu,%.4f,%.4f,%.4f,%.1f,%u,%u\n", (unsigned long)millis(), zNow(),
                  hx1.newtons(), hx2.newtons(), lastMv, phase, cycleNo);
  }
}
