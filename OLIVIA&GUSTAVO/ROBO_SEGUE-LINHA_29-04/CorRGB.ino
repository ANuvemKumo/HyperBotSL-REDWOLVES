#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <math.h>

#define TCAADDR 0x70

extern void esquerda(int tempo = 0);
extern void frente(int tempo = 0);
extern void tras(int tempo = 0);
extern void direita(int tempo = 0);
extern void parar(int tempo = 0);
// ================= SENSORES =================

Adafruit_TCS34725 tcsD(
  TCS34725_INTEGRATIONTIME_24MS,
  TCS34725_GAIN_1X
);

Adafruit_TCS34725 tcsE(
  TCS34725_INTEGRATIONTIME_24MS,
  TCS34725_GAIN_1X
);

#define LED_E = 26;
#define LED_D = 27;

// ================= HSB =================

float hue;
float saturation;
float brightness;

// ================= AUX =================

void tcaSelect(uint8_t i)
{
  if (i > 7) {return;}

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

// ================= RGB -> HSB =================

void rgbToHSB(
  uint16_t r,
  uint16_t g,
  uint16_t b,
  uint16_t c)
{
  if (c == 0)
  {
    hue = 0;
    saturation = 0;
    brightness = 0;
    return;
  }

  float R = (float)r / c;
  float G = (float)g / c;
  float B = (float)b / c;

  R = min(R, 1.0f);
  G = min(G, 1.0f);
  B = min(B, 1.0f);

  float maxVal = max(R, max(G, B));
  float minVal = min(R, min(G, B));
  float delta = maxVal - minVal;

  brightness = maxVal;

  if (maxVal == 0)
    saturation = 0;
  else
    saturation = delta / maxVal;

  if (delta == 0)
  {
    hue = 0;
  }
  else if (maxVal == R)
  {
    hue = 60.0 * fmod(((G - B) / delta), 6.0);
  }
  else if (maxVal == G)
  {
    hue = 60.0 * (((B - R) / delta) + 2.0);
  }
  else
  {
    hue = 60.0 * (((R - G) / delta) + 4.0);
  }

  if (hue < 0)
    hue += 360.0;
}

// ================= DETECÇÃO =================

bool ehVerde(uint16_t r, uint16_t g, uint16_t b)
{
  if (r == 0 || b == 0) return false;

  float rg = (float)g / r;
  float bg = (float)g / b;

  return (
    rg > 2.0 &&
    bg > 1.5
  );
}

bool ehVermelho(float hue, float sat)
{
  return (
    (hue < 20 || hue > 340) &&
    sat > 0.25
  );
}

bool ehPreto(uint16_t c)
{
  return c < 80;
}

bool ehBranco(uint16_t r, uint16_t g, uint16_t b)
{
  if (r == 0 || b == 0) return false;

  float rg = (float)g / r;
  float bg = (float)g / b;

  return (
    rg < 1.45 &&
    bg < 1.7
  );
}

// ================= LEITURAS =================

bool leuVerdeDireita()
{
  uint16_t r, g, b, c;

  tcaSelect(0);
  tcsD.getRawData(&r, &g, &b, &c);

  return ehVerde(r, g, b);
}

bool leuVerdeEsquerda()
{
  uint16_t r, g, b, c;

  tcaSelect(1);
  tcsE.getRawData(&r, &g, &b, &c);

  return ehVerde(r, g, b);
}

bool leuVermelhoDireita()
{
  uint16_t r, g, b, c;

  tcaSelect(0);
  tcsD.getRawData(&r, &g, &b, &c);

  rgbToHSB(r, g, b, c);

  return ehVermelho(hue, saturation);
}

bool leuVermelhoEsquerda()
{
  uint16_t r, g, b, c;

  tcaSelect(1);
  tcsE.getRawData(&r, &g, &b, &c);

  rgbToHSB(r, g, b, c);

  return ehVermelho(hue, saturation);
}

// ================= INICIALIZAÇÃO =================

void iniciarSensoresCor()
{
  Wire.begin();
  Wire.setClock(400000);

  tcaSelect(0);

  if (!tcsD.begin())
  {
    Serial.println("Sensor RGB direito nao encontrado");

    while (1);
  }

  tcaSelect(1);

  if (!tcsE.begin())
  {
    Serial.println("Sensor RGB esquerdo nao encontrado");

    while (1);
  }

  Serial.println("Sensores RGB iniciados");
}

// ================= AÇÕES =================

void verificarSensoresCor()
{
  bool verdeD = leuVerdeDireita();
  bool verdeE = leuVerdeEsquerda();

  bool vermelhoD = leuVermelhoDireita();
  bool vermelhoE = leuVermelhoEsquerda();

  // ================= VERMELHO =================

  if (vermelhoD || vermelhoE)
  {
    Serial.println("VERMELHO DETECTADO");

    parar(10000);

    return;
  }

  // ================= VERDE DIREITA =================

  if (verdeD && !verdeE)
  {
    Serial.println("VERDE DIREITA");

    parar(1000);
    //delay(1000);
    if (

      (sensorValues[0] > LIMIAR_PRETO &&
      sensorValues[1] > LIMIAR_PRETO &&
      sensorValues[2] > LIMIAR_PRETO) 

      ||

      (sensorValues[5] > LIMIAR_PRETO &&
      sensorValues[6] > LIMIAR_PRETO &&
      sensorValues[7] > LIMIAR_PRETO)

    ) {
        esquerda(1000);
        //delay(1000);

      }


    return;
  }

  // ================= VERDE ESQUERDA =================

  if (verdeE && !verdeD)
  {
    Serial.println("VERDE ESQUERDA");

    parar(1000);
    delay(1000);

    if (

      (sensorValues[0] > LIMIAR_PRETO &&
      sensorValues[1] > LIMIAR_PRETO &&
      sensorValues[2] > LIMIAR_PRETO) 

      ||
      
      (sensorValues[5] > LIMIAR_PRETO &&
      sensorValues[6] > LIMIAR_PRETO &&
      sensorValues[7] > LIMIAR_PRETO)

    ) {
        direita(1000);
        //delay(1000);
      }

    return;
  }

  // ================= DUPLO VERDE =================

  if (verdeD && verdeE)
  {
    Serial.println("BECO - DOIS VERDES ENCONTRADOS!");

    parar(1000);
    //delay(1000);

    if (

      (sensorValues[0] > LIMIAR_PRETO &&
      sensorValues[1] > LIMIAR_PRETO &&
      sensorValues[2] > LIMIAR_PRETO) 

      ||
      
      (sensorValues[5] > LIMIAR_PRETO &&
      sensorValues[6] > LIMIAR_PRETO &&
      sensorValues[7] > LIMIAR_PRETO)

    ) {
        direita(4300);
        //delay(2800);
      }

    return;
  }
}