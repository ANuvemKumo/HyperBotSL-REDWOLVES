// ================= SEGUIDOR PD 2 =================
int erro = 0;
int erroAnterior = 0;
int derivada = 0;

float Kp = 1;
float Kd = 0;

// velocidade base
int Vel = 190;

// limite do motor
const int VEL_MAX = 255;

// intensidade da roda reversa
const float FATOR_REVERSO = 1;

void seguirLinhaPD2()
{
    // posição da linha (0 a 7000)
    uint16_t posicao = qtr.readLineBlack(sensorValues);

    printSensoresCalibrados();

    // centro = 3500
    erro = posicao - 3500;

    // derivada
    derivada = erro - erroAnterior;

    // PD
    float correcao = (Kp * erro) + (Kd * derivada);

    erroAnterior = erro;

    // velocidades sem limitar
    int velocidadeDireita = Vel - correcao;
    int velocidadeEsquerda = Vel + correcao;

    // limita permitindo reverso
    velocidadeDireita = constrain(velocidadeDireita, -VEL_MAX, VEL_MAX);
    velocidadeEsquerda = constrain(velocidadeEsquerda, -VEL_MAX, VEL_MAX);

    // ================= MOTOR DIREITO =================

    if (velocidadeDireita >= 0)
    {
        motor_direito.run(FORWARD);
        motor_direito.setSpeed(velocidadeDireita);
    }
    else
    {
        motor_direito.run(BACKWARD);
        motor_direito.setSpeed(abs(velocidadeDireita) * FATOR_REVERSO);
    }

    // ================= MOTOR ESQUERDO =================

    if (velocidadeEsquerda >= 0)
    {
        motor_esquerdo.run(FORWARD);
        motor_esquerdo.setSpeed(velocidadeEsquerda);
    }
    else
    {
        motor_esquerdo.run(BACKWARD);
        motor_esquerdo.setSpeed(abs(velocidadeEsquerda) * FATOR_REVERSO);
    }
}