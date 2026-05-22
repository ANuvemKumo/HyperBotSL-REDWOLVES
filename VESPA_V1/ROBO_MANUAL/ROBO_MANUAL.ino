#include <Arduino.h>
#include <QTRSensors.h>
#include "AFMotor.h"

// ===== Motores =====

AF_DCMotor motor_direito(3);
AF_DCMotor motor_esquerdo(4);

// ===== Sensores QTR =====

QTRSensors qtr;

const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

uint8_t pinos[SensorCount] = {
  51,50,49,48,
  47,46,45,44
};

const uint16_t MEDIA_PRETO = 600;

// ===== Velocidade =====

#define VELOCIDADE 255

// ===== Movimentos =====

void frente() {

  motor_direito.setSpeed(VELOCIDADE);
  motor_esquerdo.setSpeed(VELOCIDADE);

  motor_direito.run(FORWARD);
  motor_esquerdo.run(FORWARD);

}

void direita() {

  motor_direito.run(RELEASE);
  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(BACKWARD);
  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(FORWARD);
}

void esquerda() {

  motor_esquerdo.run(RELEASE);
  motor_esquerdo.setSpeed(VELOCIDADE);
  motor_esquerdo.run(BACKWARD);
  motor_direito.setSpeed(VELOCIDADE);
  motor_direito.run(FORWARD);
}

void tras() {

  motor_direito.setSpeed(VELOCIDADE);
  motor_esquerdo.setSpeed(VELOCIDADE);

  motor_direito.run(BACKWARD);
  motor_esquerdo.run(BACKWARD);

}

void parar() {

  motor_direito.run(RELEASE);
  motor_esquerdo.run(RELEASE);

}

void setup() {

  motor_direito.setSpeed(VELOCIDADE);
  motor_esquerdo.setSpeed(VELOCIDADE);

}

void loop() {

  frente();
  delay(1000);

  parar();
  delay(1000);

  direita();
  delay(2000);

  parar();
  delay(1000);

  esquerda();
  delay(4000);

  parar();
  delay(1000);

  esquerda();
  delay(2000);

  parar();
  delay(1000);

  direita();
  delay(4000);

  parar();
  delay(2000);

}