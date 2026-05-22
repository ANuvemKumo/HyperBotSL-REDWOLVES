// #include <Arduino.h>
// #include <QTRSensors.h>
// #include "AFMotor.h"
// #include <math.h>

// // ================= QTR =================
// QTRSensors qtr;
// const uint8_t SensorCount = 8;
// uint16_t sensorValues[SensorCount];
// uint8_t pinos[SensorCount] = {44, 45, 46, 47, 48, 49, 50, 51};

// // ================= MOTORES =================
// AF_DCMotor motor_direito(4);
// AF_DCMotor motor_esquerdo(3);

// // ================= AJUSTES GERAIS =================
// const int VEL_MAX    = 255;
// const int VEL_MIN    = 0;
// const int VEL_MORTA  = 80;   // PWM mínimo que vence o atrito — ajuste se precisar
// int VELOCIDADE_BASE  = 180;  // subiu para garantir margem acima da zona morta

// // ✅ Fix 1: espaçamento real de 10 mm entre sensores
// const float POSICAO_CENTRO   = 3500.0f;
// const float UNIDADE_PARA_MM  = 10.0f / 1000.0f;  // 0.010 mm/unidade

// // ================= MODO DE OPERAÇÃO =================
// enum Modo { MODO_RELE, MODO_PD };
// Modo modo = MODO_RELE;

// // ================= RELÉ =================
// const int    H_RELE          = 100;
// const float  DEADBAND_MM     = 0.5f;   // aumentado para 10 mm de espaçamento
// const uint16_t CICLOS_PARA_MEDIA = 8;

// // ================= PD =================
// float Kp = 0.0f;
// float Kd = 0.0f;

// // ================= VARIÁVEIS DE MEDIÇÃO =================
// float erroAnterior_mm      = 0.0f;
// unsigned long lastLoopUs   = 0;

// bool primeiroCruzamento        = true;
// unsigned long ultimoCruzamentoUs = 0;
// float picoAtualMm              = 0.0f;

// double   somaPeriodoUs    = 0.0;
// double   somaAmplitudeMm  = 0.0;
// uint16_t ciclosValidos    = 0;

// bool ganhosCalculados      = false;
// unsigned long ultimoPrintUs = 0;

// // ================= MOVIMENTOS =================
// void aplicarMotores(int velDireita, int velEsquerda) {
//   velDireita  = constrain(velDireita,  0, VEL_MAX);
//   velEsquerda = constrain(velEsquerda, 0, VEL_MAX);

//   // Zona morta: se pediu mover mas está abaixo do mínimo efetivo, sobe
//   if (velDireita  > 0 && velDireita  < VEL_MORTA) velDireita  = VEL_MORTA;
//   if (velEsquerda > 0 && velEsquerda < VEL_MORTA) velEsquerda = VEL_MORTA;

//   motor_direito.setSpeed((uint8_t)velDireita);
//   motor_esquerdo.setSpeed((uint8_t)velEsquerda);

//   // ✅ Fix 2: ambos FORWARD — cabeamento não é espelhado neste robô
//   motor_direito.run(FORWARD);
//   motor_esquerdo.run(FORWARD);
// }

// void parar() {
//   motor_direito.run(RELEASE);
//   motor_esquerdo.run(RELEASE);
// }

// void giraDireitaCalib() {
//   motor_direito.setSpeed(VELOCIDADE_BASE);
//   motor_esquerdo.setSpeed(VELOCIDADE_BASE);
//   motor_direito.run(BACKWARD);
//   motor_esquerdo.run(FORWARD);
// }

// void giraEsquerdaCalib() {
//   motor_direito.setSpeed(VELOCIDADE_BASE);
//   motor_esquerdo.setSpeed(VELOCIDADE_BASE);
//   motor_direito.run(FORWARD);
//   motor_esquerdo.run(BACKWARD);
// }

// // ================= CALIBRAÇÃO =================
// void calibrarSensores() {
//   qtr.calibrate();

//   giraEsquerdaCalib();
//   delay(1500);
//   qtr.calibrate();

//   giraDireitaCalib();
//   delay(1500);
//   qtr.calibrate();

//   parar();
//   delay(2500);

//   giraDireitaCalib();
//   delay(1500);
//   qtr.calibrate();

//   giraEsquerdaCalib();
//   delay(1500);
//   qtr.calibrate();

//   parar();
// }

// // ================= UTILITÁRIOS =================
// float erroEmMm(uint16_t posicao) {
//   return ((float)posicao - POSICAO_CENTRO) * UNIDADE_PARA_MM;
// }

// int sinalComHisterese(float erro_mm) {
//   if (erro_mm >  DEADBAND_MM) return  1;
//   if (erro_mm < -DEADBAND_MM) return -1;
//   return 0;
// }

// void imprimirStatus(float erro_mm) {
//   unsigned long agoraUs = micros();
//   if (agoraUs - ultimoPrintUs < 200000UL) return;
//   ultimoPrintUs = agoraUs;

//   Serial.print("modo=");
//   Serial.print(modo == MODO_RELE ? "RELE" : "PD");
//   Serial.print(" | erro_mm=");
//   Serial.print(erro_mm, 3);

//   if (ciclosValidos > 0) {
//     Serial.print(" | amp_mm=");
//     Serial.print((float)(somaAmplitudeMm / ciclosValidos), 3);
//     Serial.print(" | periodo_us=");
//     Serial.print((float)(somaPeriodoUs / ciclosValidos), 1);
//   }

//   if (ganhosCalculados) {
//     Serial.print(" | Kp="); Serial.print(Kp, 4);
//     Serial.print(" | Kd="); Serial.print(Kd, 4);
//   }

//   Serial.println();
// }

// // ================= MEDIÇÃO DO RELÉ =================
// void atualizarMedicaoRele(float erro_mm) {
//   float absErro = fabsf(erro_mm);
//   if (absErro > picoAtualMm) picoAtualMm = absErro;

//   int sign = sinalComHisterese(erro_mm);

//   if (sign == 1 && !primeiroCruzamento) {
//     if (ultimoCruzamentoUs != 0) {
//       unsigned long agoraUs  = micros();
//       unsigned long periodoUs = agoraUs - ultimoCruzamentoUs;

//       somaPeriodoUs   += (double)periodoUs;
//       somaAmplitudeMm += (double)picoAtualMm;
//       ciclosValidos++;

//       Serial.print("Ciclo "); Serial.print(ciclosValidos);
//       Serial.print(" | amp_mm=");   Serial.print(picoAtualMm, 3);
//       Serial.print(" | periodo_us="); Serial.println(periodoUs);
//     }
//     ultimoCruzamentoUs = micros();
//     picoAtualMm = 0.0f;
//   }

//   if (primeiroCruzamento && sign != 0) {
//     primeiroCruzamento = false;
//     ultimoCruzamentoUs = micros();
//     picoAtualMm = 0.0f;
//   }
// }

// // ================= CÁLCULO DOS GANHOS =================
// void calcularGanhosDoRele() {
//   if (ciclosValidos < CICLOS_PARA_MEDIA) return;
//   if (ganhosCalculados) return;

//   float ampMediaMm     = (float)(somaAmplitudeMm / ciclosValidos);
//   float periodoMedioUs = (float)(somaPeriodoUs   / ciclosValidos);
//   float periodoMedioS  = periodoMedioUs / 1000000.0f;

//   float Ku = (4.0f * (float)H_RELE) / (PI * ampMediaMm);
//   Kp = 0.8f * Ku;
//   Kd = 0.1f * Ku * periodoMedioS;

//   ganhosCalculados = true;

//   Serial.println("=== GANHOS ESTIMADOS ===");
//   Serial.print("Amp media (mm): ");     Serial.println(ampMediaMm,     4);
//   Serial.print("Periodo medio (us): "); Serial.println(periodoMedioUs, 2);
//   Serial.print("Ku: ");  Serial.println(Ku, 6);
//   Serial.print("Kp: ");  Serial.println(Kp, 6);
//   Serial.print("Kd: ");  Serial.println(Kd, 6);

//   // Inicializa estado do PD antes de trocar de modo
//   erroAnterior_mm = 0.0f;
//   lastLoopUs      = micros();

//   modo = MODO_PD;
//   Serial.println("Mudando para modo PD.");
// }

// // ================= SEGUE LINHA - RELÉ =================
// void seguirLinhaRele() {
//   uint16_t posicao  = qtr.readLineBlack(sensorValues);
//   float    erro_mm  = erroEmMm(posicao);

//   atualizarMedicaoRele(erro_mm);
//   calcularGanhosDoRele();
//   imprimirStatus(erro_mm);

//   int correcao    = (erro_mm >= 0.0f) ? H_RELE : -H_RELE;
//   int velDireita  = VELOCIDADE_BASE + correcao;
//   int velEsquerda = VELOCIDADE_BASE - correcao;

//   aplicarMotores(velDireita, velEsquerda);
// }

// // ================= SEGUE LINHA - PD =================
// void seguirLinhaPD() {
//   unsigned long agoraUs = micros();
//   float dt = (agoraUs - lastLoopUs) / 1000000.0f;
//   lastLoopUs = agoraUs;
//   if (dt < 0.001f) dt = 0.001f;

//   uint16_t posicao = qtr.readLineBlack(sensorValues);
//   float    erro_mm = erroEmMm(posicao);

//   float derivada_mm_s = (erro_mm - erroAnterior_mm) / dt;
//   float correcao      = (Kp * erro_mm) + (Kd * derivada_mm_s);
//   erroAnterior_mm     = erro_mm;

//   aplicarMotores((int)(VELOCIDADE_BASE + correcao),
//                  (int)(VELOCIDADE_BASE - correcao));

//   imprimirStatus(erro_mm);
// }

// // ================= SETUP / LOOP =================
// void setup() {
//   Serial.begin(115200);

//   qtr.setTypeRC();
//   qtr.setSensorPins(pinos, SensorCount);

//   calibrarSensores();

//   lastLoopUs    = micros();
//   ultimoPrintUs = micros();

//   Serial.println("=== INICIO ===");
//   Serial.println("Modo inicial: RELE");
// }

// void loop() {
//   if (modo == MODO_RELE) {
//     seguirLinhaRele();
//   } else {
//     seguirLinhaPD();
//   }
// }