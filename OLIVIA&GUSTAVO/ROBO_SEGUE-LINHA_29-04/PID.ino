// ================= SEGUIDOR PD =================
int erro = 0;
int erroAnterior = 0;
int derivada = 0;

float Kp = 2;
float Kd = 0;

// velocidade base do segue linha
int Vel = 190;

// limite do motor
const int VEL_MAX = 255;
const int VEL_MIN = 0;

void seguirLinhaPD() {

  // posição da linha (0 a 7000)
  uint16_t posicao = qtr.readLineBlack(sensorValues);

  printSensoresCalibrados();

  // centro = 3500
  float erro = posicao - 3500;

  // derivada
  derivada = erro - erroAnterior;

  // PD
  float correcao = (Kp * erro) + (Kd * derivada);

  erroAnterior = erro;

  // ================= VELOCIDADE DOS MOTORES =================

  int velocidadeDireita = Vel - correcao;
  int velocidadeEsquerda = Vel + correcao;

  // limita velocidades
  velocidadeDireita = constrain(velocidadeDireita, VEL_MIN, VEL_MAX);
  velocidadeEsquerda = constrain(velocidadeEsquerda, VEL_MIN, VEL_MAX);

  // aplica velocidades
  motor_direito.setSpeed(velocidadeDireita);
  motor_esquerdo.setSpeed(velocidadeEsquerda);

  // ambos para frente
  motor_direito.run(FORWARD);
  motor_esquerdo.run(FORWARD);
}