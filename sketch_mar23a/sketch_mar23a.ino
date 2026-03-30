#include <RoboCore_Vespa.h>

// ==================== MOTORES ====================
VespaMotors motores;

// Velocidade base do robô
const int VELOCIDADE_BASE = 50;
const int VELOCIDADE_MAX = 70;
const int LIMITE_CONTROLE = 50;

// ==================== SENSORES ====================
// Mesmos pinos que você forneceu
#define EXT_DIR 32  // S4
#define DIR     33  // S3
#define ESQ     25  // S2
#define EXT_ESQ 26  // S1

// ==================== PID ====================
// Constantes PID (PRECISAM SER AJUSTADAS!)
float Kp = 25.0;
float Ki = 0.0;
float Kd = 15.0;

// Variáveis PID
float erro = 0;
float erroAnterior = 0;
float integral = 0;
float derivada = 0;
float controle = 0;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);

  pinMode(EXT_ESQ, INPUT);
  pinMode(ESQ, INPUT);
  pinMode(DIR, INPUT);
  pinMode(EXT_DIR, INPUT);

  Serial.println("Seguidor de linha com PID iniciado!");
}

// ==================== LOOP ====================
void loop() {

  // Leitura dos sensores (0 ou 1)
  int extEsq = digitalRead(EXT_ESQ);
  int esq    = digitalRead(ESQ);
  int dir    = digitalRead(DIR);
  int extDir = digitalRead(EXT_DIR);

  // ==================== CÁLCULO DO ERRO ====================
  // Atribuímos pesos para cada sensor
  // Linha no meio = erro 0
  // Esquerda negativa | Direita positiva

  if (extEsq == 1 && esq == 0 && dir == 0 && extDir == 0) erro = -3;
  else if (extEsq == 1 && esq == 1 && dir == 0 && extDir == 0) erro = -2;
  else if (extEsq == 0 && esq == 1 && dir == 0 && extDir == 0) erro = -1;
  else if (extEsq == 0 && esq == 1 && dir == 1 && extDir == 0) erro = 0;
  else if (extEsq == 0 && esq == 0 && dir == 1 && extDir == 0) erro = 1;
  else if (extEsq == 0 && esq == 0 && dir == 1 && extDir == 1) erro = 2;
  else if (extEsq == 0 && esq == 0 && dir == 0 && extDir == 1) erro = 3;
  // Caso perca a linha, mantém último erro
  else erro = erroAnterior;

  // ==================== PID ====================
  integral += erro;
  derivada = erro - erroAnterior;

  controle = (Kp * erro) + (Ki * integral) + (Kd * derivada);

  // 🔥 LIMITADOR DO PID (ESSENCIAL)
  controle = constrain(controle, -LIMITE_CONTROLE, LIMITE_CONTROLE);

  erroAnterior = erro;

  // ==================== VELOCIDADE DOS MOTORES ====================
  int velocidadeEsq = VELOCIDADE_BASE + controle;
  int velocidadeDir = VELOCIDADE_BASE - controle;

  // Limitar velocidades
  velocidadeEsq = constrain(velocidadeEsq, -VELOCIDADE_MAX, VELOCIDADE_MAX);
  velocidadeDir = constrain(velocidadeDir, -VELOCIDADE_MAX, VELOCIDADE_MAX);

  // Aplicar nos motores
  motores.setSpeedLeft(velocidadeEsq);
  motores.setSpeedRight(velocidadeDir);

  // ==================== DEBUG ====================
  Serial.print("Erro: ");
  Serial.print(erro);
  Serial.print(" | Controle: ");
  Serial.print(controle);
  Serial.print(" | Esq: ");
  Serial.print(velocidadeEsq);
  Serial.print(" | Dir: ");
  Serial.println(velocidadeDir);

  delay(10);
}