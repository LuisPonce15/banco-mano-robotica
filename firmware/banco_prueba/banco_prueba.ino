// ============================================================
//  Banco E-01 · PRUEBA DE HARDWARE (paso 1)
//  Sin librerías extra. Se maneja desde el Monitor Serie a 115200.
//  Funciona con el paquete "esp32 by Espressif" 2.x o 3.x.
//
//  Comandos (escribe y presiona Enter):
//    ?          ayuda
//    s          estado de finales de carrera, botón y motores
//    m          monitor en vivo de entradas (cualquier tecla para salir)
//    i          buscar los INA219 en el bus I2C
//    e / d      habilitar / soltar los motores (EN)
//    a 800      mover el eje A 800 pasos  (a -800 = sentido contrario)
//    b 800      mover el eje B 800 pasos
//    v 800      velocidad en pasos por segundo (50 a 4000)
//    1 90       servo 1 (DS3240) a 90°
//    2 90       servo 2 (DS3235) a 90°
//    x          quitar la señal a los servos
//  Cualquier tecla durante un movimiento lo detiene.
// ============================================================
#include <Wire.h>

const int STEP_A = 25, DIR_A = 26;
const int STEP_B = 32, DIR_B = 33;
const int EN_PIN = 27;
const int FC_A = 18, FC_B = 19, BTN_PIN = 23;
const int SV1_PIN = 16, SV2_PIN = 17;
const int SDA_PIN = 21, SCL_PIN = 22;

// Direcciones I2C de los INA219 según los puentes A0/A1 de cada módulo:
//   sin puentes = 0x40 · A0 = 0x41 · A1 = 0x44 · A0 y A1 = 0x45
const uint8_t INA1_ADDR = 0x40;   // INA1 · servo DS3240
const uint8_t INA2_ADDR = 0x41;   // INA2 · servo DS3235

int  stepsPerSec = 800;
bool enabled = false;
bool svAttached[2] = {false, false};

// NC a GND con pull-up: HIGH = presionado (o cable suelto)
bool pressed(int pin) { return digitalRead(pin) == HIGH; }

void printStatus() {
  Serial.printf("FC-A: %-8s  FC-B: %-8s  BTN: %-10s  Motores: %s\n",
                pressed(FC_A) ? "TOCADO" : "libre",
                pressed(FC_B) ? "TOCADO" : "libre",
                pressed(BTN_PIN) ? "PRESIONADO" : "libre",
                enabled ? "con fuerza" : "sueltos");
}

void setEnable(bool on) {
  enabled = on;
  digitalWrite(EN_PIN, on ? LOW : HIGH);
  Serial.println(on ? "Motores HABILITADOS (EN = LOW)" : "Motores sueltos (EN = HIGH)");
}

void moveSteps(char axis, long n) {
  int stepPin = (axis == 'a') ? STEP_A : STEP_B;
  int dirPin  = (axis == 'a') ? DIR_A  : DIR_B;
  int fc      = (axis == 'a') ? FC_A   : FC_B;
  if (!enabled)          { Serial.println("Primero habilita los motores con 'e'."); return; }
  if (pressed(BTN_PIN))  { Serial.println("BTN presionado: no muevo nada.");       return; }
  if (n == 0)            { Serial.println("Escribe cuántos pasos, por ejemplo: a 800"); return; }
  n = constrain(n, -32000L, 32000L);

  digitalWrite(dirPin, n > 0 ? HIGH : LOW);
  delayMicroseconds(20);
  bool fcAtStart = pressed(fc);
  uint32_t halfUs = 500000UL / stepsPerSec;
  long done = 0;
  for (long i = 0; i < labs(n); i++) {
    if (pressed(BTN_PIN))             { Serial.println("Detenido por BTN.");                  break; }
    if (!fcAtStart && pressed(fc))    { Serial.println("Detenido: final de carrera tocado.");  break; }
    if (Serial.available())           { while (Serial.available()) Serial.read();
                                        Serial.println("Detenido por teclado.");               break; }
    digitalWrite(stepPin, HIGH); delayMicroseconds(halfUs);
    digitalWrite(stepPin, LOW);  delayMicroseconds(halfUs);
    done++;
    if (i % 1000 == 999) vTaskDelay(1);   // deja respirar al sistema en movimientos largos
  }
  Serial.printf("Eje %c: %ld de %ld pasos (DIR = %s)\n",
                axis == 'a' ? 'A' : 'B', n > 0 ? done : -done, n, n > 0 ? "HIGH" : "LOW");
}

void servoWrite(int idx, int angle) {
  int pin = (idx == 0) ? SV1_PIN : SV2_PIN;
  angle = constrain(angle, 0, 180);
  if (!svAttached[idx]) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (!ledcAttach(pin, 50, 14)) { Serial.println("Error al configurar el PWM del servo."); return; }
#else
    ledcSetup(idx, 50, 14);          // canal 0 = servo 1, canal 1 = servo 2
    ledcAttachPin(pin, idx);
#endif
    svAttached[idx] = true;
  }
  uint32_t us   = 500 + (uint32_t)angle * 2000 / 180;   // 500–2500 µs
  uint32_t duty = us * 16383UL / 20000UL;               // periodo de 20 ms, 14 bits
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(idx, duty);
#endif
  Serial.printf("Servo %d (%s) -> %d grados (%lu us)\n", idx + 1,
                idx == 0 ? "DS3240" : "DS3235", angle, (unsigned long)us);
}

void servosOff() {
  for (int i = 0; i < 2; i++) {
    if (svAttached[i]) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
      ledcDetach(i == 0 ? SV1_PIN : SV2_PIN);
#else
      ledcDetachPin(i == 0 ? SV1_PIN : SV2_PIN);
#endif
      svAttached[i] = false;
    }
  }
  Serial.println("Servos sin señal (quedan sueltos).");
}

void i2cScan() {
  Serial.println("Buscando dispositivos I2C...");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      const char* tag = "";
      if (a == INA1_ADDR)                          tag = "  <- INA1 (DS3240)";
      else if (a == INA2_ADDR)                     tag = "  <- INA2 (DS3235)";
      else if (a >= 0x40 && a <= 0x4F)             tag = "  <- probablemente un INA219 (dirección no esperada)";
      Serial.printf("  0x%02X%s\n", a, tag);
      found++;
    }
  }
  if (!found) Serial.println("  Nada. Revisa SDA = 21, SCL = 22, 3V3 y GND de los INA219.");
  else if (found == 1) Serial.println("  Solo hay uno: si conectaste dos INA219, seguramente tienen la misma dirección (puentes A0/A1 iguales).");
}

void monitorInputs() {
  Serial.println("Monitor de entradas: presiona los finales de carrera y el botón. Cualquier tecla para salir.");
  while (Serial.available()) Serial.read();
  int last = -1;
  while (!Serial.available()) {
    int st = pressed(FC_A) | (pressed(FC_B) << 1) | (pressed(BTN_PIN) << 2);
    if (st != last) { printStatus(); last = st; }
    delay(20);
  }
  while (Serial.available()) Serial.read();
  Serial.println("Fin del monitor.");
}

void help() {
  Serial.println();
  Serial.println("=== Banco E-01 · prueba de hardware ===");
  Serial.println(" ?        ayuda");
  Serial.println(" s        estado de entradas y motores");
  Serial.println(" m        monitor en vivo de entradas");
  Serial.println(" i        buscar INA219 (I2C)");
  Serial.println(" e / d    habilitar / soltar motores");
  Serial.println(" a 800    mover eje A (negativo = al revés)");
  Serial.println(" b 800    mover eje B");
  Serial.println(" v 800    velocidad en pasos/s");
  Serial.println(" 1 90     servo DS3240 a 90 grados");
  Serial.println(" 2 90     servo DS3235 a 90 grados");
  Serial.println(" x        quitar señal a los servos");
  Serial.println("Cualquier tecla durante un movimiento lo detiene.");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);                 // arranca con motores sueltos
  pinMode(STEP_A, OUTPUT); pinMode(DIR_A, OUTPUT);
  pinMode(STEP_B, OUTPUT); pinMode(DIR_B, OUTPUT);
  digitalWrite(STEP_A, LOW); digitalWrite(STEP_B, LOW);
  pinMode(FC_A, INPUT_PULLUP);
  pinMode(FC_B, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(300);
  help();
  printStatus();
}

void loop() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (!line.length()) return;
  delay(5);
  while (Serial.available()) Serial.read();

  char c   = tolower(line[0]);
  long val = line.length() > 1 ? line.substring(1).toInt() : 0;

  switch (c) {
    case '?': help(); break;
    case 's': printStatus(); break;
    case 'm': monitorInputs(); break;
    case 'i': i2cScan(); break;
    case 'e': setEnable(true); break;
    case 'd': setEnable(false); break;
    case 'a':
    case 'b': moveSteps(c, val); break;
    case 'v': stepsPerSec = constrain(val, 50L, 4000L);
              Serial.printf("Velocidad: %d pasos/s\n", stepsPerSec); break;
    case '1':
    case '2': servoWrite(c - '1', (int)val); break;
    case 'x': servosOff(); break;
    default:  Serial.println("No entendí. Escribe ? para ver los comandos.");
  }
}
