#include <Arduino.h>
#include <RoboCore_Vespa.h>
#include <Wire.h>
#include <BluetoothSerial.h>

// ================= BLUETOOTH =================
BluetoothSerial SerialBT;
const char* BT_REMOTE_NAME = "ESP32_RGB";
const char* BT_LOCAL_NAME  = "VESPA_LINE";

// ================= MOTORES =================
VespaMotors motores;

int VELOCIDADE_BASE = 55; //65
int VELOCIDADE_MAX = 70;
int LIMITE_CONTROLE = 65;

// Velocidade dos giros por cor
int VELOCIDADE_GIRO = 60;

// ================= SENSORES =================
#define EXT_DIR 32
#define DIR     33
#define ESQ     25
#define EXT_ESQ 26

int extEsq, esq, dir, extDir;

// ================= PID =================
float Kp = 25.0; //45
float Ki = 0.0;
float Kd = 15.0;

float erro = 0;
float erroAnterior = 0;
float integral = 0;
float derivada = 0;
float controle = 0;

// ================= ESTADO BT/COR =================
char ultimoComandoBT = 'N'; // ultimo comando valido lido
bool bloqueioVermelho = false;
unsigned long inicioBloqueioVermelho = 0;
unsigned long fimUltimaAcaoCor = 0;

const uint32_t PARADA_VERMELHO_MS = 20000; // 20s
const uint32_t DELAY_ENTRE_ACOES_MS = 120;

// ================= ACOES MOTORES (CORES) =================
void pararMotores() {
  motores.setSpeedLeft(0);
  motores.setSpeedRight(0);
}

void girarEsquerda90() {
  motores.setSpeedLeft(-VELOCIDADE_GIRO);
  motores.setSpeedRight(VELOCIDADE_GIRO);
  delay(400);
  pararMotores();
}

void girarDireita90() {
  motores.setSpeedLeft(VELOCIDADE_GIRO);
  motores.setSpeedRight(-VELOCIDADE_GIRO);
  delay(400);
  pararMotores();
}

void giro180() {
  motores.setSpeedLeft(VELOCIDADE_GIRO);
  motores.setSpeedRight(-VELOCIDADE_GIRO);
  delay(800);
  pararMotores();
}

// ================= BLUETOOTH =================
bool estadoValidoBT(char c) {
  return (c == 'E' || c == 'D' || c == 'R' || c == 'C' || c == 'N');
}

void tentaConectarBT() {
  if (SerialBT.connected()) return;
  SerialBT.connect(BT_REMOTE_NAME);
}

// Lê a fila e guarda apenas o último comando válido
void atualizarUltimoComandoDaFilaBT() {
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();
    if (estadoValidoBT(c)) {
      ultimoComandoBT = c;
    }
  }
}

// Retorna true se executou/esta executando acao de cor
bool processarAcoesCorViaBT() {
  if (!SerialBT.connected()) {
    tentaConectarBT();
  }

  atualizarUltimoComandoDaFilaBT();

  // Parada de 20s quando ambos vermelhos
  if (bloqueioVermelho) {
    pararMotores();
    if (millis() - inicioBloqueioVermelho >= PARADA_VERMELHO_MS) {
      bloqueioVermelho = false;
      fimUltimaAcaoCor = millis();
    }
    return true;
  }

  // Delay entre acoes
  if (millis() - fimUltimaAcaoCor < DELAY_ENTRE_ACOES_MS) {
    return false;
  }

  switch (ultimoComandoBT) {
    case 'E':
      girarEsquerda90();
      fimUltimaAcaoCor = millis();
      atualizarUltimoComandoDaFilaBT();
      return true;

    case 'D':
      girarDireita90();
      fimUltimaAcaoCor = millis();
      atualizarUltimoComandoDaFilaBT();
      return true;

    case 'R':
      giro180();
      fimUltimaAcaoCor = millis();
      atualizarUltimoComandoDaFilaBT();
      return true;

    case 'C':
      pararMotores();
      bloqueioVermelho = true;
      inicioBloqueioVermelho = millis();
      return true;

    case 'N':
    default:
      return false;
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(EXT_ESQ, INPUT);
  pinMode(ESQ, INPUT);
  pinMode(DIR, INPUT);
  pinMode(EXT_DIR, INPUT);

  // BT em modo master para conectar no ESP32 dos sensores
  SerialBT.begin(BT_LOCAL_NAME, true);
  tentaConectarBT();
}

// ================= LOOP =================
void loop() {

  // Se houver acao de cor (giro/parada 20s), prioriza isso neste ciclo.
  if (processarAcoesCorViaBT()) {
    delay(10);
    return;
  }

  // ======= LOGICA ORIGINAL DO SEGUIDOR (inalterada) =======
  extEsq = digitalRead(EXT_ESQ);
  esq    = digitalRead(ESQ);
  dir    = digitalRead(DIR);
  extDir = digitalRead(EXT_DIR);

  if (extEsq == 1 && esq == 0 && dir == 0 && extDir == 0) erro = -3;
  else if (extEsq == 1 && esq == 1 && dir == 0 && extDir == 0) erro = -2;
  else if (extEsq == 0 && esq == 1 && dir == 0 && extDir == 0) erro = -1;
  else if (extEsq == 0 && esq == 1 && dir == 1 && extDir == 0) erro = 0;
  else if (extEsq == 0 && esq == 0 && dir == 1 && extDir == 0) erro = 1;
  else if (extEsq == 0 && esq == 0 && dir == 1 && extDir == 1) erro = 2;
  else if (extEsq == 0 && esq == 0 && dir == 0 && extDir == 1) erro = 3;
  else erro = 0;

  integral += erro;
  derivada = erro - erroAnterior;

  controle = (Kp * erro) + (Ki * integral) + (Kd * derivada);
  controle = constrain(controle, -LIMITE_CONTROLE, LIMITE_CONTROLE);

  erroAnterior = erro;

  int velocidadeEsq = VELOCIDADE_BASE + controle;
  int velocidadeDir = VELOCIDADE_BASE - controle;

  velocidadeEsq = constrain(velocidadeEsq, -VELOCIDADE_MAX, VELOCIDADE_MAX);
  velocidadeDir = constrain(velocidadeDir, -VELOCIDADE_MAX, VELOCIDADE_MAX);

  motores.setSpeedLeft(velocidadeEsq);
  motores.setSpeedRight(velocidadeDir);

  delay(10);
}