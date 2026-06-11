// ================= SEGUIDOR PD 2 =================
int erro = 0;
int erroAnterior = 0;
int derivada = 0;

float Kp = 2;
float Kd = 0;

const int VEL_NORMAL = 190;
const int VEL_RAMPA  = 230;
int velocidadeBase;

const int VEL_MAX = 255;

const float FATOR_REVERSO = 1;

extern bool estaNaRampa(float limite = 15);

void seguirLinhaPD2()
{
    uint16_t posicao = qtr.readLineBlack(sensorValues);

    erro = posicao - 3500;

    derivada = erro - erroAnterior;

    //int velocidadeBase;
    float kpAtual;

    if (estaNaRampa())
    {
        velocidadeBase = VEL_RAMPA;
        kpAtual = 0.6; // correção mais suave na rampa

        Serial.println("RAMPA");
    }
    else
    {
        velocidadeBase = VEL_NORMAL;
        kpAtual = Kp;
    }

    float correcao = (kpAtual * erro) + (Kd * derivada);

    erroAnterior = erro;

    int velocidadeDireita = velocidadeBase - correcao;
    int velocidadeEsquerda = velocidadeBase + correcao;

    velocidadeDireita = constrain(
        velocidadeDireita,
        -VEL_MAX,
        VEL_MAX
    );

    velocidadeEsquerda = constrain(
        velocidadeEsquerda,
        -VEL_MAX,
        VEL_MAX
    );

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