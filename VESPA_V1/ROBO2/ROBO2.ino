#include <Arduino.h>
#include <QTRSensors.h>

// ================= QTR =================
QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
uint8_t pinos[SensorCount] = {51, 50, 49, 48, 47, 46, 45, 44};

// ================= MOTORES =================
#define IN1 22
#define IN2 23
#define IN3 24
#define IN4 25

// ================= AJUSTES =================
const uint16_t LIMIAR_PRETO = 600;    // preto ~1000, branco ~0 (readCalibrated)
const uint16_t TIMEOUT_LADO = 4000;   // tempo max de busca por lado (ms)
const uint16_t T60_MS = 100;          // ajuste para ~60 graus no seu robô

int ultimoLado = 1; // -1 esquerda, 1 direita

// ================= MOVIMENTOS =================

void frente() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); 
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void giraDireita() { // uso no seguidor
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void giraEsquerda() { // uso no seguidor
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void parar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// Movimentos dedicados de calibração (giro contínuo)
void giraDireitaCalib() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void giraEsquerdaCalib() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// ================= DEBUG SERIAL =================
void printSensoresCalibrados() {
  Serial.print("S: ");
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(sensorValues[i]); // 0..1000
    if (i < SensorCount - 1) Serial.print('\t');
  }
  Serial.println();
}

// ================= DETECÇÃO =================
bool todosEsquerdaPreto() { // sensores 0..3
  for (uint8_t i = 0; i < 4; i++) {
    if (sensorValues[i] < LIMIAR_PRETO) return false;
  }
  return true;
}

bool todosDireitaPreto() { // sensores 4..7
  for (uint8_t i = 4; i < 8; i++) {
    if (sensorValues[i] < LIMIAR_PRETO) return false;
  }
  return true;
}

// ================= CALIBRAÇÃO =================
bool varrerAteEsquerdaPreto() {
  // gira para direita até os sensores da esquerda verem preto
  unsigned long t0 = millis();

  while (millis() - t0 < TIMEOUT_LADO) {
    giraDireitaCalib();
    qtr.calibrate();
    qtr.readCalibrated(sensorValues);

    printSensoresCalibrados();

    if (todosEsquerdaPreto()) {
      parar();
      Serial.println("OK: lado esquerdo preto detectado.");
      return true;
    }

    delay(20); // deixa leitura legível no serial
  }

  parar();
  Serial.println("Timeout procurando lado esquerdo preto.");
  return false;
}

bool varrerAteDireitaPreto() {
  // gira para esquerda até os sensores da direita verem preto
  unsigned long t0 = millis();

  while (millis() - t0 < TIMEOUT_LADO) {
    giraEsquerdaCalib();
    qtr.calibrate();
    qtr.readCalibrated(sensorValues);

    printSensoresCalibrados();

    if (todosDireitaPreto()) {
      parar();
      Serial.println("OK: lado direito preto detectado.");
      return true;
    }

    delay(20);
  }

  parar();
  Serial.println("Timeout procurando lado direito preto.");
  return false;
}

void fallback60Graus() {
  Serial.println("Fallback: varredura aproximada de 60 graus por lado.");

  // +60 direita
  giraDireitaCalib();
  for (uint16_t i = 0; i < T60_MS / 5; i++) {
    qtr.calibrate();
    qtr.readCalibrated(sensorValues);
    printSensoresCalibrados();
    delay(5);
  }
  parar();
  delay(80);

  // -120 esquerda (passa do centro e vai 60 para outro lado)
  giraEsquerdaCalib();
  for (uint16_t i = 0; i < (2 * T60_MS) / 5; i++) {
    qtr.calibrate();
    qtr.readCalibrated(sensorValues);
    printSensoresCalibrados();
    delay(5);
  }
  parar();
  delay(80);

  // +60 direita para voltar próximo ao centro
  giraDireitaCalib();
  for (uint16_t i = 0; i < T60_MS / 5; i++) {
    qtr.calibrate();
    qtr.readCalibrated(sensorValues);
    printSensoresCalibrados();
    delay(5);
  }
  parar();
  delay(80);
}

void calibrarAntesDeSeguir() {
  Serial.println("=== INICIO CALIBRACAO ===");

  // Pré-calibração parado
  for (uint8_t i = 0; i < 40; i++) {
    qtr.calibrate();
    qtr.readCalibrated(sensorValues);
    printSensoresCalibrados();
    delay(10);
  }

  bool okEsquerda = varrerAteEsquerdaPreto();
  delay(100);
  bool okDireita = varrerAteDireitaPreto();

  if (!(okEsquerda && okDireita)) {
    fallback60Graus();
  }

  parar();
  Serial.println("=== FIM CALIBRACAO ===");
  delay(150);
}

// ================= SEGUIDOR (sem PWM) =================
void seguirLinha() {
  qtr.readCalibrated(sensorValues);
  printSensoresCalibrados(); // mantenha para debug; remova depois se quiser

  bool s0 = sensorValues[0] > LIMIAR_PRETO;
  bool s1 = sensorValues[1] > LIMIAR_PRETO;
  bool s2 = sensorValues[2] > LIMIAR_PRETO;
  bool s3 = sensorValues[3] > LIMIAR_PRETO;
  bool s4 = sensorValues[4] > LIMIAR_PRETO;
  bool s5 = sensorValues[5] > LIMIAR_PRETO;
  bool s6 = sensorValues[6] > LIMIAR_PRETO;
  bool s7 = sensorValues[7] > LIMIAR_PRETO;

  // Centro
  if (s3 || s4 || (s2 && s5)) {
    frente();
    ultimoLado = 0;
  }
  // Esquerda
  else if (s0 || s1 || s2) {
    giraDireita();
    ultimoLado = -1;
  }
  // Direita
  else if (s5 || s6 || s7) {
    giraEsquerda();
    ultimoLado = 1;
  }
  // Perdeu linha
  else {
    if (ultimoLado == -1) giraEsquerda();
    else giraDireita();
  }
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  qtr.setTypeRC();
  qtr.setSensorPins(pinos, SensorCount);

  calibrarAntesDeSeguir();
}

void loop() {
  seguirLinha();
  delay(10);
}