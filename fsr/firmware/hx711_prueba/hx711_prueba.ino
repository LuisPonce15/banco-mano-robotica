// ============================================================
//  Banco FSR · PRUEBA DE LAS CELDAS DE CARGA (2× HX711)
//  Sin librerías. Mismos pines que fsr_ensayo:
//    HX711-1 (celda 1): DT = GPIO16, SCK = GPIO17
//    HX711-2 (celda 2): DT = GPIO18, SCK = GPIO19
//
//  Al arrancar revisa si cada HX711 responde y hace la tara.
//  Luego imprime las cuentas crudas, el cambio desde la tara y un
//  peso aproximado (con el factor teórico, sin calibrar).
//
//  Monitor Serie a 115200 ("Nueva línea"):
//    t   tara (sin nada encima)
//    p   modo Serial Plotter      x   modo texto
//    d   diagnóstico de nuevo
// ============================================================
#include <Arduino.h>

struct Cell {
  const char* name; int dt, sck;
  long raw, off; bool ok; uint32_t n, tLast;
  double s, s2; int ns; float noise;
  Cell(const char* nm, int d, int k) : name(nm), dt(d), sck(k), raw(0), off(0), ok(false), n(0), tLast(0), s(0), s2(0), ns(0), noise(0) {}
};
Cell c1("Celda 1", 16, 17), c2("Celda 2", 18, 19);
const float COUNTS_PER_G = 214.8f;   // teórico: 10 kg, 1 mV/V, ganancia 128 (≈ 21 900 cuentas/N)
char mode = 'x';
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void say(const String& s) { Serial.print("# "); Serial.println(s); }

bool readCell(Cell& c) {
  if (digitalRead(c.dt) != LOW) return false;   // LOW = hay un dato listo
  uint32_t v = 0;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < 24; i++) {
    digitalWrite(c.sck, HIGH); delayMicroseconds(1);
    v = (v << 1) | (uint32_t)digitalRead(c.dt);
    digitalWrite(c.sck, LOW);  delayMicroseconds(1);
  }
  digitalWrite(c.sck, HIGH); delayMicroseconds(1);   // pulso 25: canal A, ganancia 128
  digitalWrite(c.sck, LOW);  delayMicroseconds(1);
  portEXIT_CRITICAL(&mux);
  if (v & 0x800000) v |= 0xFF000000;
  c.raw = (int32_t)v; c.n++; c.tLast = millis();
  return true;
}

// Mira 1,5 s cómo se comporta DT y cuántas lecturas salen
void diagnose(Cell& c) {
  int lows = 0, samples = 0; c.n = 0;
  uint32_t t0 = millis(); long mn = 2147483647L, mx = -2147483647L;
  while (millis() - t0 < 1500) {
    if (digitalRead(c.dt) == LOW) lows++;
    samples++;
    if (readCell(c)) { mn = min(mn, c.raw); mx = max(mx, c.raw); }
    delayMicroseconds(200);
  }
  float rate = c.n / 1.5f;
  c.ok = c.n >= 5;
  if (!c.ok) {
    if (lows == 0) say(String(c.name) + ": NO RESPONDE. DT siempre en HIGH: revisa VCC = 3,3 V y GND del módulo, y que DT vaya a GPIO" + c.dt + " y SCK a GPIO" + c.sck + " (no al revés).");
    else           say(String(c.name) + ": DT siempre en LOW: corto a GND en DT, o DT y SCK cruzados.");
    return;
  }
  say(String(c.name) + ": responde, " + String(rate, 1) + " lecturas/s (10 = RATE a GND, 80 = RATE a VCC).");
  if (mx >= 8388600 || mn <= -8388600) say(String(c.name) + ": SATURADO (" + mx + "). Revisa los 4 hilos de la celda: rojo E+, negro E−, blanco A−, verde A+.");
  else if (mx - mn > 20000) say(String(c.name) + ": la lectura salta mucho sin carga (" + (mx - mn) + " cuentas). Revisa que la celda no esté tocando nada y los contactos.");
}

void tare() {
  Cell* cs[2] = {&c1, &c2};
  for (Cell* c : cs) {
    if (!c->ok) continue;
    double a = 0; int k = 0; uint32_t t0 = millis();
    while (k < 16 && millis() - t0 < 3000) { if (readCell(*c)) { a += c->raw; k++; } delay(1); }
    if (k) c->off = lround(a / k);
  }
  say("Tara lista. Pon peso (o presiona con el dedo) sobre cada celda: el valor en g debe subir.");
}

void diagAll() {
  say("Diagnóstico de los HX711 (no toques las celdas)...");
  diagnose(c1); diagnose(c2);
  tare();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  Cell* cs[2] = {&c1, &c2};
  for (Cell* c : cs) { pinMode(c->dt, INPUT_PULLUP); pinMode(c->sck, OUTPUT); digitalWrite(c->sck, LOW); }
  delay(600);   // el HX711 tarda ~400 ms en dar el primer dato
  say("=== Banco FSR · prueba de celdas de carga ===");
  say("t tara · p Serial Plotter · x texto · d diagnóstico");
  diagAll();
}

void loop() {
  if (Serial.available()) {
    String l = Serial.readStringUntil('\n'); l.trim();
    if (l == "t") tare(); else if (l == "p" || l == "x") mode = l[0]; else if (l == "d") diagAll();
  }
  readCell(c1); readCell(c2);

  // ruido de cada celda (desviación de las últimas 20 lecturas)
  static long last1 = 0, last2 = 0;
  Cell* cs[2] = {&c1, &c2};
  for (Cell* c : cs) {
    if (c->ok && c->tLast && c->raw != (c == &c1 ? last1 : last2)) {
      (c == &c1 ? last1 : last2) = c->raw;
      double d = c->raw - c->off; c->s += d; c->s2 += d * d; c->ns++;
      if (c->ns >= 20) { double m = c->s / c->ns; c->noise = sqrt(max(0.0, c->s2 / c->ns - m * m)); c->s = c->s2 = 0; c->ns = 0; }
    }
    if (c->ok && millis() - c->tLast > 1500) { c->ok = false; say(String(c->name) + " dejó de responder."); }
  }

  static uint32_t tp = 0;
  if (millis() - tp < (mode == 'p' ? 100u : 500u)) return;
  tp = millis();
  float g1 = (c1.raw - c1.off) / COUNTS_PER_G, g2 = (c2.raw - c2.off) / COUNTS_PER_G;
  if (mode == 'p') {
    Serial.printf("Celda1_g:%.1f,Celda2_g:%.1f\n", c1.ok ? g1 : 0, c2.ok ? g2 : 0);
  } else {
    Serial.printf("C1: %s raw %9ld  Δ %8ld  ≈ %8.1f g  ruido %5.0f | C2: %s raw %9ld  Δ %8ld  ≈ %8.1f g  ruido %5.0f\n",
                  c1.ok ? "ok" : "--", c1.raw, c1.raw - c1.off, g1, c1.noise,
                  c2.ok ? "ok" : "--", c2.raw, c2.raw - c2.off, g2, c2.noise);
  }
}
