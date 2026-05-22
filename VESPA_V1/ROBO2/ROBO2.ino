#include <Arduino.h>
#include <QTRSensors.h>

// ================= QTR =================
QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
uint8_t pinos[SensorCount] = {44, 45, 46, 47, 48, 49, 50, 51};

// ================= MOTORES =================
#define IN1 22
#define IN2 23
#define IN3 24
#define IN4 25

const uint16_t LIMIAR_PRETO = 900;
int ultimoLado = 1;

// ================= MOVIMENTOS =================
void frente()      { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);  }
void giraDireita() { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH); }
void giraEsquerda(){ digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH); digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);  }
void parar()       { digitalWrite(IN1,LOW);  digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,LOW);  }

// ================= CALIBRAÇÃO =================
void calibrarAntesDeSeguir() {
  Serial.println("=== INICIO CALIBRACAO ===");

  // Parado — referência inicial
  digitalWrite(IN1,LOW);  digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,LOW);
  for (uint8_t i = 0; i < 20; i++) { qtr.calibrate(); delay(20); }

  // Varre para a direita
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
  delay(1000);
  for (uint8_t i = 0; i < 20; i++) { qtr.calibrate(); delay(20); }

  // Varre para a esquerda (2x — volta ao centro e vai para o outro lado)
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH); digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  delay(2000);
  for (uint8_t i = 0; i < 20; i++) { qtr.calibrate(); delay(20); }

  // Volta ao centro
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
  delay(1000);

  // Para
  digitalWrite(IN1,LOW);  digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,LOW);

  Serial.println("=== FIM CALIBRACAO ===");
  delay(500);
}

// ================= SEGUIDOR =================
void seguirLinha() {
  qtr.readCalibrated(sensorValues);

  bool s0 = sensorValues[0] > LIMIAR_PRETO;
  bool s1 = sensorValues[1] > LIMIAR_PRETO;
  bool s2 = sensorValues[2] > LIMIAR_PRETO;
  bool s3 = sensorValues[3] > LIMIAR_PRETO;
  bool s4 = sensorValues[4] > LIMIAR_PRETO;
  bool s5 = sensorValues[5] > LIMIAR_PRETO;
  bool s6 = sensorValues[6] > LIMIAR_PRETO;
  bool s7 = sensorValues[7] > LIMIAR_PRETO;

  if      (s3 || s4)           { frente();       }
  else if (s0 || s1 || s2)     { giraEsquerda(); ultimoLado = -1; }
  else if (s5 || s6 || s7)     { giraDireita();  ultimoLado =  1; }
  else {
    if (ultimoLado == -1) giraDireita();
    else                  giraEsquerda();
  }
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  qtr.setTypeRC();
  qtr.setSensorPins(pinos, SensorCount);

  calibrarAntesDeSeguir();
}

void loop() {
  seguirLinha();
  delay(10);
}