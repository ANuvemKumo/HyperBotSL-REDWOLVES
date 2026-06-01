#include <Arduino.h>
#include <QTRSensors.h>
#include "AFMotor.h"

// ================= QTR =================
QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
uint8_t pinos[SensorCount] = {44, 45, 46, 47, 48, 49, 50, 51};

// ================= MOTORES =================
AF_DCMotor motor_direito(4);
AF_DCMotor motor_esquerdo(3);

// ================= AJUSTES =================
const uint16_t LIMIAR_PRETO = 800;    // preto ~1000, branco ~0 (readCalibrated)
const uint16_t TIMEOUT_LADO = 4000;   // tempo max de busca por lado (ms)
int ultimoLado = 1; // -1 esquerda, 1 direita

int VELOCIDADE = 255;

// ================= ULTRASSONICO ================
const int trigPin = 41;
const int echoPin = 40; //PORTAS TEMPORARIAS

const int limiteCm = 15;
bool D_direita = true;
bool D_esquerda = false;

// ================= MOVIMENTOS =================

void parar(int tempo = 0) {
  motor_direito.run(RELEASE);
  motor_esquerdo.run(RELEASE);

  if (tempo > 0){
    delay(tempo);
  }
}

void frente(int tempo = 0) {

  motor_direito.setSpeed(VELOCIDADE);
  motor_esquerdo.setSpeed(VELOCIDADE);

  motor_direito.run(FORWARD);
  motor_esquerdo.run(FORWARD);

  if (tempo > 0){
    delay(tempo);
  }
}

void direita(int tempo = 0) {
  motor_direito.run(RELEASE);
  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(BACKWARD);
  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(FORWARD);

  if (tempo > 0){
    delay(tempo);
  }
}

void direitaForte(int tempo = 0) {

  motor_direito.run(RELEASE);

  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(FORWARD);

  if (tempo > 0){
    delay(tempo);
  }
}

void esquerdaForte(int tempo = 0) {

  motor_esquerdo.run(RELEASE);

  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(FORWARD);

  if (tempo > 0){
    delay(tempo);
  }
}

void esquerda(int tempo = 0) {

  motor_esquerdo.run(RELEASE);
  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(BACKWARD);
  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(FORWARD);

  if (tempo > 0){
    delay(tempo);
  }
}

void tras(int tempo = 0) {

  motor_direito.setSpeed(VELOCIDADE);
  motor_esquerdo.setSpeed(VELOCIDADE);

  motor_direito.run(BACKWARD);
  motor_esquerdo.run(BACKWARD);

  if (tempo > 0){
    delay(tempo);
  }
}

// Movimentos dedicados de calibração (giro contínuo)
void giraDireitaCalib() {
  motor_direito.run(RELEASE);
  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(BACKWARD);
  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(FORWARD);
}

void giraEsquerdaCalib() {
  motor_esquerdo.run(RELEASE);
  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(BACKWARD);
  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(FORWARD);
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

void calibra() {
  Serial.println("============== CALIBRANDO... ==============");
  for (uint16_t i = 0; i < 400; i++)
  {
    Serial.println(i);
    qtr.calibrate();
  }
  Serial.println("============= CALIBRADO! =============");
  delay(500);
}

void calibraAE() {
  qtr.calibrate();
  qtr.readCalibrated(sensorValues);
  esquerda();
  delay(1500);
  qtr.calibrate();
  qtr.readCalibrated(sensorValues);
  direita();
  delay(1500);
  qtr.calibrate();
  qtr.readCalibrated(sensorValues);
  parar();
  delay(2500);
  direita();
  delay(1200);
  qtr.calibrate();
  qtr.readCalibrated(sensorValues);
  esquerda();
  delay(1500);
  qtr.calibrate();
  qtr.readCalibrated(sensorValues);
  parar();
  delay(2500);
}


bool varrerAteEsquerdaPreto() {
  // gira para direita até os sensores da esquerda verem preto
  unsigned long t0 = millis();

  while (millis() - t0 < TIMEOUT_LADO) {
    // giraDireitaCalib();
    direita();
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
    // giraEsquerdaCalib();
    esquerda();
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
  if (s3 || s4 ) {
    frente();
    //ultimoLado = 0;
  }
  // Esquerda
  else if (s0 || s1 || s2) {
    esquerda();
    ultimoLado = -1;
  }
  // Direita
  else if (s5 || s6 || s7) {
    direita();
    ultimoLado = 1;
  }
  // Perdeu linha
  else {
    if (ultimoLado == -1) direita();
    else esquerda();
  }
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  qtr.setTypeRC();
  qtr.setSensorPins(pinos, SensorCount);

  //calibrarAntesDeSeguir();
  calibra();
  iniciarSensoresCor();  
}

void loop() {
  verificarSensoresCor();
  detectarObstaculo();
  seguirLinhaPD();
  delay(0);
}