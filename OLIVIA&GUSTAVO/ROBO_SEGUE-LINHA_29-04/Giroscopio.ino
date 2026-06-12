#include <Wire.h>
#include <math.h>

#define MMA8452_ADDRESS 0x1C

#define TCA_ADDR 0x70
#define TCA_CHANNEL 2

const float LIMIAR = 30.0;

// ================= OFFSETS =================
float rollOffset = 0;
float pitchOffset = 0;

// ================= VALORES ATUAIS =================
float rollAtual = 0;
float pitchAtual = 0;

// ================= REGISTRADORES =================
byte readRegister(byte reg)
{
  tcaSelect(TCA_CHANNEL);

  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MMA8452_ADDRESS, 1);

  if (Wire.available())
    return Wire.read();

  return 0;
}

void writeRegister(byte reg, byte value)
{
  tcaSelect(TCA_CHANNEL);

  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// ================= LEITURA DOS ÂNGULOS =================
void lerAngulos(float &roll, float &pitch)
{
  tcaSelect(TCA_CHANNEL);

  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(0x01);
  Wire.endTransmission(false);

  Wire.requestFrom(MMA8452_ADDRESS, 6);

  if (Wire.available() != 6)
    return;

  int16_t x = (Wire.read() << 8) | Wire.read();
  int16_t y = (Wire.read() << 8) | Wire.read();
  int16_t z = (Wire.read() << 8) | Wire.read();

  x >>= 4;
  y >>= 4;
  z >>= 4;

  if (x > 2047) x -= 4096;
  if (y > 2047) y -= 4096;
  if (z > 2047) z -= 4096;

  float ax = x / 1024.0;
  float ay = y / 1024.0;
  float az = z / 1024.0;

  // Roll  -> esquerda/direita
  // Pitch -> frente/trás

  roll = atan2(ax, az) * 180.0 / PI;

  pitch = atan2(
            ay,
            sqrt(ax * ax + az * az)
          ) * 180.0 / PI;
}

// ================= CALIBRAÇÃO =================
void calibrarZero()
{
  Serial.println("Calibrando MMA8452...");

  float rSum = 0;
  float pSum = 0;

  const int N = 50;

  for (int i = 0; i < N; i++)
  {
    float r, p;

    lerAngulos(r, p);

    rSum += r;
    pSum += p;

    delay(20);
  }

  rollOffset = rSum / N;
  pitchOffset = pSum / N;

  Serial.print("Roll Offset: ");
  Serial.println(rollOffset);

  Serial.print("Pitch Offset: ");
  Serial.println(pitchOffset);

  Serial.println("Calibracao concluida");
}

// ================= INICIALIZAÇÃO =================
void iniciarGiroscopio()
{
  tcaSelect(TCA_CHANNEL);

  byte whoami = readRegister(0x0D);

  Serial.print("WHO_AM_I = 0x");
  Serial.println(whoami, HEX);

  if (whoami != 0x2A)
  {
    Serial.println("ERRO: MMA8452 nao encontrado!");
    return;
  }

  // Standby
  writeRegister(0x2A, 0x00);

  // ±2g
  writeRegister(0x0E, 0x00);

  // Ativa
  writeRegister(0x2A, 0x01);

  delay(100);

  calibrarZero();
}

// ================= ATUALIZAÇÃO =================
void atualizarGiroscopio()
{
  lerAngulos(rollAtual, pitchAtual);

  rollAtual -= rollOffset;
  pitchAtual -= pitchOffset;
}

// ================= GETTERS =================
float getRoll()
{
  return rollAtual;
}

float getPitch()
{
  return pitchAtual;
}

// ================= DETECÇÕES =================
bool inclinadoEsquerda()
{
  return getRoll() > LIMIAR;
}

bool inclinadoDireita()
{
  return getRoll() < -LIMIAR;
}

bool inclinadoFrente()
{
  return getPitch() > LIMIAR;
}

bool inclinadoTras()
{
  return getPitch() < -LIMIAR;
}

// ================= RAMPA =================
bool estaNaRampa(float limite = 15)
{
  return abs(getPitch()) > limite;
}

// ================= DEBUG =================
void imprimirGiroscopio()
{
  Serial.print("Roll: ");
  Serial.print(getRoll());

  Serial.print("\tPitch: ");
  Serial.println(getPitch());
}