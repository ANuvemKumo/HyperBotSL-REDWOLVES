#include <Arduino.h>
#include <QTRSensors.h>

// =========== Definição dos Sensores QTR ========== //

QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
uint8_t pinos[SensorCount] = {51,50,49,48,47,46,45,44};

// ============= Motores ================ //

#define IN1 22
#define IN2 23
#define IN3 24
#define IN4 25

// ============= Variáveis Responsáveis pelos dados do QTR ============ //

const uint16_t MEDIA_PRETO = 600; //O preto completo é 1000 e o branco é próx de 0

// ================= MOVIMENTOS =================

/*

Regras de movimentação de Motores com Ponte-H

| INx1 | INx2 | Resultado                  |
| ---- | ---- | -------------------------- |
| HIGH | LOW  | Gira pra frente            |
| LOW  | HIGH | Gira pra trás              |
| LOW  | LOW  | Desligado             |
| HIGH | HIGH | Freio (depende da ponte H) |

*/

void frente() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void direita() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void esquerda() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void parar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void setup() {
}

void loop() {
  frente();
  delay(500);
  parar();
  delay(1000);
  direita();
  delay(1000);
  parar();
  delay(1000);
  esquerda();
  delay(2000)
  parar();
  delay(1000);
}
