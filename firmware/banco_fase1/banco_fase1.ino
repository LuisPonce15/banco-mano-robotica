// ============================================================
//  Banco E-01 · Fase 1: ejes M1 + M2 controlados por WiFi
//  ESP32 DevKit V1 + 2× A4988 + finales de carrera + botón BTN
//
//  El ESP32 crea su propia red WiFi y sirve una página de control.
//  Conéctate a la red "BancoE01" y abre http://192.168.4.1
// ============================================================
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FastAccelStepper.h>

// ---------------- AJUSTES (revísalos antes de cargar) ----------------
const char* AP_SSID   = "BancoE01";
const char* AP_PASS   = "mano2026";   // mínimo 8 caracteres
const float LEAD_MM   = 8.0;          // avance de la varilla por vuelta (mm). Mídelo.
const int   MICROSTEP = 16;           // 16 si MS1–MS3 van a 3,3 V; 1 si quedaron libres
const float STROKE_MM = 200.0;        // carrera útil de cada eje (mm)
const bool  INVERT_A  = false;        // true si el eje A se aleja del motor al hacer homing
const bool  INVERT_B  = false;        // igual para el eje B
const float HOME_FAST = 5.0;          // mm/s, primera búsqueda del final de carrera
const float HOME_SLOW = 1.0;          // mm/s, segundo toque lento
const float HOME_BACK = 2.0;          // mm que retrocede entre los dos toques

// ---------------- PINES (iguales al plano E-01) ----------------
const int STEP_A = 25, DIR_A = 26;
const int STEP_B = 32, DIR_B = 33;
const int EN_PIN = 27;                // ENABLE de ambos A4988 (LOW = con fuerza)
const int FC_A   = 18, FC_B = 19;     // finales de carrera, contacto NC a GND
const int BTN_PIN = 23;               // botón de paro por software, contacto NC a GND

const float STEPS_PER_MM = 200.0f * MICROSTEP / LEAD_MM;

// ---------------- Página de control (vive dentro del ESP32) ----------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Banco E-01 · control</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#f3f5f8;color:#18212d}
.card{background:#fff;border:1px solid #d3d9e1;border-radius:8px;padding:14px;margin:0 auto 12px;max-width:560px}
h1{font-size:20px;margin:0 0 4px}
.st{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}
.st div{border:1px solid #d3d9e1;border-radius:6px;padding:6px;font-size:12px;color:#556173}
.st b{display:block;font:500 16px ui-monospace,monospace;color:#18212d}
button{font:inherit;padding:8px 12px;border-radius:6px;border:1px solid #18212d;background:#fff;margin:3px 3px 3px 0}
button.red{background:#c42f24;color:#fff;border-color:#c42f24}
input[type=range]{width:100%;margin:6px 0 12px}
.err{color:#c42f24;font-size:14px;min-height:1.2em}.ok{color:#1f7a45}
.note{font-size:12px;color:#556173}
</style></head><body>
<div class="card"><h1>Banco E-01 · ejes M1 + M2</h1><div id="con">Conectando…</div><div class="err" id="err"></div></div>
<div class="card"><div class="st">
<div>Eje A<b id="xa">—</b></div><div>Eje B<b id="xb">—</b></div><div>Estado<b id="est">—</b></div>
<div>FC-A<b id="fa">—</b></div><div>FC-B<b id="fb">—</b></div><div>Motores<b id="en">—</b></div>
</div><p class="note">La posición es la que el ESP32 comandó (no hay encoder). Si un motor pierde pasos, solo el homing lo corrige.</p></div>
<div class="card">
<label>Posición objetivo: <b id="tv">0</b> mm</label>
<input type="range" id="t" min="0" max="200" value="0" step="1">
<div><button onclick="go(0)">Inicio</button><button onclick="go(S/2)">Mitad</button><button onclick="go(S)">Fin</button></div>
<label>Velocidad: <b id="vv">20</b> mm/s</label><input type="range" id="v" min="1" max="60" value="20">
<label>Aceleración: <b id="av">200</b> mm/s²</label><input type="range" id="a" min="20" max="1000" step="10" value="200">
</div>
<div class="card">
<button onclick="send({c:'en',on:1})">Habilitar motores</button>
<button onclick="send({c:'en',on:0})">Deshabilitar</button>
<button onclick="send({c:'home'})">Homing</button>
<button class="red" onclick="send({c:'stop'})">Detener</button>
<button onclick="send({c:'rearm'})">Rearmar paro</button>
</div>
<script>
let ws,S=200;const $=i=>document.getElementById(i);
function send(o){if(ws&&ws.readyState==1)ws.send(JSON.stringify(o))}
function go(mm){$('t').value=mm;$('tv').textContent=Math.round(mm);send({c:'move',mm:+mm})}
$('t').oninput=e=>$('tv').textContent=e.target.value;
$('t').onchange=e=>go(+e.target.value);
function lbl(){$('vv').textContent=$('v').value;$('av').textContent=$('a').value}
function spd(){lbl();send({c:'spd',v:+$('v').value,a:+$('a').value})}
$('v').oninput=lbl;$('a').oninput=lbl;$('v').onchange=spd;$('a').onchange=spd;
function conn(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=()=>{$('con').innerHTML='<span class="ok">Conectado al ESP32</span>';spd()};
  ws.onclose=()=>{$('con').textContent='Desconectado, reintentando…';setTimeout(conn,1000)};
  ws.onmessage=e=>{const d=JSON.parse(e.data);S=d.stroke;$('t').max=S;
    $('xa').textContent=d.xa.toFixed(2)+' mm';$('xb').textContent=d.xb.toFixed(2)+' mm';
    $('est').textContent=d.sw?'PARO BTN':d.homing?'homing…':!d.homed?'sin homing':d.run?'moviendo':'quieto';
    $('fa').textContent=d.fa?'tocado':'libre';$('fb').textContent=d.fb?'tocado':'libre';
    $('en').textContent=d.en?'con fuerza':'sueltos';$('err').textContent=d.err||''}
}
conn();
</script></body></html>
)rawliteral";

// ---------------- Estado ----------------
FastAccelStepperEngine engine;
FastAccelStepper *stA = nullptr, *stB = nullptr;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct Msg { char txt[160]; };
QueueHandle_t cmdQ;

enum HomeState { H_IDLE, H_FAST, H_STOP1, H_BACK, H_SLOW, H_STOP2, H_DONE, H_ERR };
HomeState hA = H_IDLE, hB = H_IDLE;

bool enabled = false, swStop = false, homed = false, homing = false;
float vmax = 20, amax = 200;   // mm/s y mm/s²
String lastErr = "";

// ---------------- Utilidades ----------------
bool fcPressed(int pin) { return digitalRead(pin) == HIGH; }  // NC a GND: HIGH = tocado o cable suelto
long mm2st(float mm)    { return lroundf(mm * STEPS_PER_MM); }
float st2mm(long s)     { return s / STEPS_PER_MM; }

void setEnable(bool on) {
  enabled = on;
  digitalWrite(EN_PIN, on ? LOW : HIGH);
}

void applySpeed(FastAccelStepper* s, float v, float a) {
  s->setSpeedInHz((uint32_t)max(1.0f, v * STEPS_PER_MM));
  s->setAcceleration((int32_t)max(1.0f, a * STEPS_PER_MM));
}

void stopAll(bool hard) {
  if (hard) { stA->forceStop(); stB->forceStop(); }
  else      { stA->stopMove();  stB->stopMove();  }
  homing = false;
  hA = hB = H_IDLE;
}

void moveBoth(float mm) {
  if (swStop)   { lastErr = "Paro por software activo: rearma primero"; return; }
  if (!enabled) { lastErr = "Motores deshabilitados";                   return; }
  if (!homed)   { lastErr = "Haz homing antes de mover";                return; }
  if (homing)   return;
  lastErr = "";
  mm = constrain(mm, 0.0f, STROKE_MM);
  applySpeed(stA, vmax, amax);
  applySpeed(stB, vmax, amax);
  long target = mm2st(mm);
  stA->moveTo(target);   // los dos reciben la misma orden, una tras otra (microsegundos)
  stB->moveTo(target);
}

// ---------------- Homing de un eje (máquina de estados) ----------------
void homeStep(FastAccelStepper* s, int fc, HomeState& h) {
  switch (h) {
    case H_FAST:
      if (fcPressed(fc)) { s->forceStop(); h = H_STOP1; }
      else if (s->getCurrentPosition() < -mm2st(STROKE_MM + 20)) { s->forceStop(); h = H_ERR; }
      break;
    case H_STOP1:
      if (!s->isRunning()) {
        s->setCurrentPosition(0);
        applySpeed(s, HOME_FAST, 200);
        s->moveTo(mm2st(HOME_BACK));
        h = H_BACK;
      }
      break;
    case H_BACK:
      if (!s->isRunning() && s->getCurrentPosition() == mm2st(HOME_BACK)) {
        applySpeed(s, HOME_SLOW, 100);
        s->runBackward();
        h = H_SLOW;
      }
      break;
    case H_SLOW:
      if (fcPressed(fc)) { s->forceStop(); h = H_STOP2; }
      else if (s->getCurrentPosition() < -mm2st(10)) { s->forceStop(); h = H_ERR; }
      break;
    case H_STOP2:
      if (!s->isRunning()) { s->setCurrentPosition(0); h = H_DONE; }
      break;
    default: break;
  }
}

void startHoming() {
  if (swStop) { lastErr = "Paro por software activo: rearma primero"; return; }
  setEnable(true);
  homed = false; homing = true; lastErr = "";
  applySpeed(stA, HOME_FAST, 200); stA->runBackward();
  applySpeed(stB, HOME_FAST, 200); stB->runBackward();
  hA = hB = H_FAST;
}

// ---------------- Comandos desde la página ----------------
void handleCmd(const char* txt) {
  JsonDocument d;
  if (deserializeJson(d, txt)) return;
  const char* c = d["c"] | "";
  if (!strcmp(c, "move")) {
    moveBoth(d["mm"] | 0.0f);
  } else if (!strcmp(c, "spd")) {
    vmax = constrain(d["v"] | vmax, 1.0f, 100.0f);
    amax = constrain(d["a"] | amax, 10.0f, 2000.0f);
  } else if (!strcmp(c, "en")) {
    bool on = (d["on"] | 0) != 0;
    if (swStop && on) { lastErr = "Paro por software activo: rearma primero"; return; }
    if (!on) { stopAll(true); homed = false; }   // sueltos = la posición ya no es confiable
    setEnable(on);
    lastErr = "";
  } else if (!strcmp(c, "home")) {
    startHoming();
  } else if (!strcmp(c, "stop")) {
    stopAll(false);
  } else if (!strcmp(c, "rearm")) {
    if (digitalRead(BTN_PIN) == HIGH) lastErr = "Suelta el botón (o revisa su cable) antes de rearmar";
    else { swStop = false; lastErr = "Rearmado: habilita los motores y haz homing"; }
  }
}

// Llega en la tarea de red: solo se copia a la cola; se procesa en loop()
void onWs(AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type,
          void* arg, uint8_t* data, size_t len) {
  if (type != WS_EVT_DATA) return;
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) return;
  Msg m;
  size_t n = min(len, sizeof(m.txt) - 1);
  memcpy(m.txt, data, n);
  m.txt[n] = 0;
  xQueueSend(cmdQ, &m, 0);
}

void sendState() {
  JsonDocument d;
  d["xa"] = st2mm(stA->getCurrentPosition());
  d["xb"] = st2mm(stB->getCurrentPosition());
  d["run"] = stA->isRunning() || stB->isRunning();
  d["en"] = enabled;  d["homed"] = homed;  d["homing"] = homing;  d["sw"] = swStop;
  d["fa"] = fcPressed(FC_A);  d["fb"] = fcPressed(FC_B);
  d["btn"] = digitalRead(BTN_PIN) == HIGH;
  d["v"] = vmax;  d["a"] = amax;  d["stroke"] = STROKE_MM;
  d["err"] = lastErr;
  String s;
  serializeJson(d, s);
  ws.textAll(s);
}

// ---------------- setup / loop ----------------
void setup() {
  Serial.begin(115200);
  pinMode(EN_PIN, OUTPUT);
  setEnable(false);                      // arranca con los motores sueltos
  pinMode(FC_A, INPUT_PULLUP);
  pinMode(FC_B, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);

  engine.init();
  stA = engine.stepperConnectToPin(STEP_A);
  stB = engine.stepperConnectToPin(STEP_B);
  if (!stA || !stB) {
    Serial.println("Error: no pude configurar los pines STEP");
    while (true) delay(1000);
  }
  stA->setDirectionPin(DIR_A, !INVERT_A);
  stB->setDirectionPin(DIR_B, !INVERT_B);
  applySpeed(stA, vmax, amax);
  applySpeed(stB, vmax, amax);

  cmdQ = xQueueCreate(8, sizeof(Msg));

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Red WiFi: "); Serial.print(AP_SSID);
  Serial.print("   Abre: http://"); Serial.println(WiFi.softAPIP());

  ws.onEvent(onWs);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", INDEX_HTML); });
  server.begin();
}

void loop() {
  Msg m;
  while (xQueueReceive(cmdQ, &m, 0) == pdTRUE) handleCmd(m.txt);

  // Botón de paro por software (NC a GND: HIGH = presionado o cable suelto)
  if (digitalRead(BTN_PIN) == HIGH && !swStop) {
    swStop = true;
    stopAll(true);
    setEnable(false);
    homed = false;
    lastErr = "Paro por software (BTN). Suelta el botón y pulsa Rearmar.";
  }

  if (homing) {
    homeStep(stA, FC_A, hA);
    homeStep(stB, FC_B, hB);
    if (hA == H_ERR || hB == H_ERR) {
      stopAll(true);
      lastErr = "Homing: no encontré el final de carrera. ¿El eje va hacia el motor? Revisa INVERT_A/B.";
    } else if (hA == H_DONE && hB == H_DONE) {
      homing = false; homed = true; hA = hB = H_IDLE;
      lastErr = "";
    }
  } else {
    // Protección: final de carrera tocado mientras el eje va hacia el motor
    if (fcPressed(FC_A) && stA->isRunning() &&
        stA->getPositionAfterCommandsCompleted() < stA->getCurrentPosition()) {
      stopAll(true); lastErr = "FC-A tocado en movimiento: detenido";
    }
    if (fcPressed(FC_B) && stB->isRunning() &&
        stB->getPositionAfterCommandsCompleted() < stB->getCurrentPosition()) {
      stopAll(true); lastErr = "FC-B tocado en movimiento: detenido";
    }
  }

  static uint32_t tState = 0, tClean = 0;
  if (millis() - tState >= 50)  { tState = millis(); if (ws.count()) sendState(); }
  if (millis() - tClean >= 1000) { tClean = millis(); ws.cleanupClients(); }
}
