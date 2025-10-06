#include <DistanceSensor.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h" // Biblioteca do leitor RGB TCS34725

// =======================
// Pinos dos motores
#define MOTOR_ESQUERDO 9
#define MOTOR_DIREITO 11
#define DIRECAO_ESQUERDA 8
#define DIRECAO_DIREITA 10

// Pinos dos sensores digitais (linha)
#define SENSOR_EXTREMO_ESQUERDO A0
#define SENSOR_ESQUERDO A1
#define SENSOR_DIREITO A2
#define SENSOR_EXTREMO_DIREITO A3

#define LIMIAR 0.5

int ledPin = 5; // Pino PWM ligando o led central (pode ser qualquer pino PWM do Arduino)
int brilho = 0; // Nível de brilho do LED (valor entre 0 e 255)

// Parâmetros PID
float Kp = 30; // Pode variar
float Ki = 0;
float Kd = 2;

float setPoint = 0; // Se refere ao valor que o PID se baseará para corrigir o erro
float erro = 0; // É a variação de um sensor para o outro. Em teoria será 1, 0 e -1 no digital e de 0 a 100 no analogico
float erro_anterior = 0; // Guarda o erro anterior ao loop atual e será usado de comparação no PID
float integral = 0; // Guarda a soma de todos os erros
float output = 0; // É a saída dos valores PID após correção do erro

float P = 0, D = 0;

// Controla o giro dos motores (0 - 255) o que aumenta a velocidade
int VELOCIDADE = 80; // Controla a velocidade do carrinho
int VELOCIDADE_MINIMA = 0;
int VELOCIDADE_MAXIMA = 255;
int velocidade_esquerda = 0;
int velocidade_direita = 0;

// Variáveis usadas para calcular o tempo de leitura dos sensores
unsigned long tempoInicioEsquerda = 0;
bool aguardandoGiroEsquerda = false;

unsigned long tempoInicioDireita = 0;
bool aguardandoGiroDireita = false;

//PINOS sensor ultrassônico
const int echoPin = 2;
const int trigPin = 3;

// Start the sensor
DistanceSensor sensor(trigPin, echoPin);

// Váriaveis sensor ultrassônico
bool Obstaculo = false;

// =======================
// Configuração dos sensores TCS34725 via TCA9548A
#define TCAADDR 0x70

Adafruit_TCS34725 tcsEsquerdo = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X); // Integra o sensor esquerdo com a biblioteca que define um tempo de inegração em MS e um ganho de X vezes
Adafruit_TCS34725 tcsDireito = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X); // Integra o sensor direita e faz o mesmo que foi citado acima

uint16_t rE, gE, bE, cE; // Guarda os valores RGB do sensor esquerdo
uint16_t rD, gD, bD, cD; // Guarda os valores RGB do sensor direito

// ======================= EXPLICAÇÃO DA FUNÇÃO ABAIXO ======================= //
/*
  A função tcaSelect ativa um canal específico do multiplexador TCA9548A usando a comunicação I2C.
  O valor i passado como argumento seleciona qual canal será ativado (de 0 a 7).
  Quando um canal é ativado, o Arduino pode se comunicar com os dispositivos conectados a esse canal
*/

void tcaSelect(uint8_t i) {
  if (i > 7) return; // Verifica se o índice fornecido está dentro do intervalo válido (0 a 7), caso contrário, sai da função;

  Wire.beginTransmission(TCAADDR); // Inicia a transmissão I2C com o endereço do multiplexador (TCAADDR)

  Wire.write(1 << i); // Envia um comando para ativar o canal selecionado, deslocando o valor '1' para a posição 'i'
                      // Isso cria um número binário com um único bit '1' na posição correspondente ao canal

  Wire.endTransmission(); // Finaliza a transmissão I2C, enviando o comando para o multiplexador
}

// ==== Variáveis para leitura assíncrona sensores cor ====
unsigned long tempoInicioLEDEsquerdo = 0; // Define um tempo de inicio para ligar os leds -- Importante para calibração caso necessário
bool ledEsquerdoAceso = false; // Define se o led está ligado == true ou desligado == false
bool leituraProntaEsquerdo = false; // Usado para guardar a informação se o sensor já fez uma leitura

// Definilções acima só que para o RGB direito
unsigned long tempoInicioLEDDireito = 0;
bool ledDireitoAceso = false;
bool leituraProntaDireito = false;


// Função do led para sensor esquerdo
void atualizaLedDesligadoEsquerdo() {
  tcaSelect(0); // Canal do sensor esquerdo
  tcsEsquerdo.getRawData(&rE, &gE, &bE, &cE); // Valores RGB brutos dos sensores
  tcsEsquerdo.setInterrupt(false); // Apaga LED
  leituraProntaEsquerdo = true; // Completa a leitura do sensor
}

// Função do led para sensor direito
void atualizaLedDesligadoDireito() {
  tcaSelect(1); // Canal do sensor direito
  tcsDireito.getRawData(&rD, &gD, &bD, &cD); // Valores RGB brutos dos sensores
  tcsDireito.setInterrupt(false); // Apaga LED
  leituraProntaDireito = true; // Completa a leitura do sensor
}

// ======================= SETUP ======================= //

void setup() {
  pinMode(MOTOR_ESQUERDO, OUTPUT);
  pinMode(MOTOR_DIREITO, OUTPUT);
  pinMode(DIRECAO_ESQUERDA, OUTPUT);
  pinMode(DIRECAO_DIREITA, OUTPUT);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);

  // Start serial port
  Serial.begin(115200); 
  Serial.println("Iniciando sistema..."); 

  // Definidores para led central
  pinMode(ledPin, OUTPUT); // Define o pino como saída (saída digital)
  analogWrite(ledPin, brilho); // Define o valor do brilho no LED (controla o PWM)

  //Serial.begin(9600); // Conecta o arduino com o canal 9600 do Serial Monitor
  Wire.begin();

  tcaSelect(0);
  if (!tcsEsquerdo.begin()) {
    Serial.println("Sensor de cor ESQUERDO não encontrado.");
    while (1);
  }

  tcaSelect(1);
  if (!tcsDireito.begin()) {
    Serial.println("Sensor de cor DIREITO não encontrado.");
    while (1);
  }

  Serial.println("Sensores de cor inicializados.");
}

// ======================= Funções de giro ======================= //

void ReCurta() {
  digitalWrite(DIRECAO_ESQUERDA, HIGH);
  digitalWrite(DIRECAO_DIREITA, HIGH);
  analogWrite(MOTOR_ESQUERDO, 10);
  analogWrite(MOTOR_DIREITO, 10);
  delay(1);

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
}

void MarchaCurta() {
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  analogWrite(MOTOR_ESQUERDO, 10);
  analogWrite(MOTOR_DIREITO, 10);
  delay(1);

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
}

void giroCurvaFechadaEsquerda() {
  const unsigned long tempoMinimo = 300; // Define o tempo em que o carrinho irá girar para o lado (Isso vai definir o ângulo do giro)
  unsigned long inicio = millis(); // O millis é uma função que guarda em si o tempo em que o arduino está ligado
  bool passouTempoMinimo = false;

  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, HIGH);
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  delay(1);

  while (true) {
    if (millis() - inicio >= tempoMinimo) passouTempoMinimo = true;

    float sensorE = analogRead(SENSOR_ESQUERDO) / 1023.0;
    float sensorD = analogRead(SENSOR_DIREITO) / 1023.0;

    if (passouTempoMinimo || (sensorD > LIMIAR)) break;
  }

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
}

// Giro do VERDE para a esquerda(ajustado)
void giroCurvaFechadaEsquerdaPLUS() {
  const unsigned long tempoMinimo = 500; // Define o tempo em que o carrinho irá girar para o lado (Isso vai definir o ângulo do giro)
  unsigned long inicio = millis(); // O millis é uma função que guarda em si o tempo em que o arduino está ligado
  bool passouTempoMinimo = false;

  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, HIGH);
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  delay(1);

  while (true) {
    if (millis() - inicio >= tempoMinimo) passouTempoMinimo = true;

    float sensorE = analogRead(SENSOR_ESQUERDO) / 1023.0;
    float sensorD = analogRead(SENSOR_DIREITO) / 1023.0;

    if (passouTempoMinimo) break;
  }

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);

  corrigeLeituras();
}

void giroCurvaFechadaDireita() {
  const unsigned long tempoMinimo = 300;
  unsigned long inicio = millis();
  bool passouTempoMinimo = false;

  digitalWrite(DIRECAO_ESQUERDA, HIGH);
  digitalWrite(DIRECAO_DIREITA, LOW);
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  delay(1);

  while (true) {
    if (millis() - inicio >= tempoMinimo) passouTempoMinimo = true;

    float sensorE = analogRead(SENSOR_ESQUERDO) / 1023.0;
    float sensorD = analogRead(SENSOR_DIREITO) / 1023.0;

    if (passouTempoMinimo || (sensorE > LIMIAR)) break;
  }

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
}

// Giro do VERDE para a direita(ajustado)
void giroCurvaFechadaDireitaPLUS() {
  const unsigned long tempoMinimo = 500;
  unsigned long inicio = millis();
  bool passouTempoMinimo = false;

  digitalWrite(DIRECAO_ESQUERDA, HIGH);
  digitalWrite(DIRECAO_DIREITA, LOW);
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  delay(1);

  while (true) {
    if (millis() - inicio >= tempoMinimo) passouTempoMinimo = true;

    float sensorE = analogRead(SENSOR_ESQUERDO) / 1023.0;
    float sensorD = analogRead(SENSOR_DIREITO) / 1023.0;

    if (passouTempoMinimo) break;
  }

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);

  corrigeLeituras();
}

// Função do retorno
void giroRetorno() {
  const unsigned long tempoMinimo = 1600;
  unsigned long inicio = millis();
  bool passouTempoMinimo = false;

  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, HIGH);
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO,150);
  delay(1);

  while (true) {
    if (millis() - inicio >= tempoMinimo) passouTempoMinimo = true;

    float sensorE = analogRead(SENSOR_ESQUERDO) / 1023.0;
    float sensorD = analogRead(SENSOR_DIREITO) / 1023.0;

    if (passouTempoMinimo) break;

  }
    analogWrite(MOTOR_ESQUERDO, 0);
    analogWrite(MOTOR_DIREITO, 0);
    digitalWrite(DIRECAO_ESQUERDA, LOW);
    digitalWrite(DIRECAO_DIREITA, LOW);
    
}

//Função do desvio de obstáculos
void Desvio(){
  
  //Para o robô
  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  //Ré
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  digitalWrite(DIRECAO_ESQUERDA, HIGH);
  digitalWrite(DIRECAO_DIREITA, HIGH);
  delay(500);

  //Para o robô
  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  //Virar para direita
  giroCurvaFechadaDireitaPLUS();
  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  //Andar para frente
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  //Virar para esquerda
  giroCurvaFechadaEsquerdaPLUS();
  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  //Andar para frente
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 150);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);

  //Virar para esquerda
  giroCurvaFechadaEsquerdaPLUS();
  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
  delay(1000);
  
  //Enquanto o sensor interno direito não ver preto faz
  unsigned long tempoInicio = millis();
  unsigned long duracao = 1000; // 1 segundo
  bool andando = true;

  while (andando) {
    float sensorE = analogRead(SENSOR_ESQUERDO) / 1023.0;
    float sensorD = analogRead(SENSOR_DIREITO) / 1023.0;
    if ((millis() - tempoInicio < duracao) && (sensorD <= LIMIAR)) {
      // Anda para frente
      analogWrite(MOTOR_ESQUERDO, 120);
      analogWrite(MOTOR_DIREITO, 120);
      digitalWrite(DIRECAO_ESQUERDA, LOW);
      digitalWrite(DIRECAO_DIREITA, LOW);
      
      // Aqui você pode ler sensores ou fazer outras coisas
    } else {
      // Parar os motores após o tempo acabar
      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      digitalWrite(DIRECAO_ESQUERDA, LOW);
      digitalWrite(DIRECAO_DIREITA, LOW);
      delay(1000);
      andando = false;
    }
  }
  
  //Virar para direita
  giroCurvaFechadaDireita();
}

// Função para normalizar os valores RGB
void normalizaRGB(uint16_t &r, uint16_t &g, uint16_t &b) {
  // Encontra o valor máximo dos três canais
  uint16_t maxValor = max(max(r, g), b);

  // Se o valor máximo for maior que 255, normalizamos
  if (maxValor > 255) {
    float fator = 255.0 / maxValor;  // Calcula o fator de normalização
    r = r * fator;
    g = g * fator;
    b = b * fator;
  }
}

// Função para corrigir e normalizar as leituras dos sensores
void corrigeLeituras() {
  // Normaliza os valores dos sensores
  normalizaRGB(rE, gE, bE);
  normalizaRGB(rD, gD, bD);

  // Limitar valores para garantir que não ultrapassem 255
  rE = (rE > 255) ? 255 : rE;
  gE = (gE > 255) ? 255 : gE;
  bE = (bE > 255) ? 255 : bE;

  rD = (rD > 255) ? 255 : rD;
  gD = (gD > 255) ? 255 : gD;
  bD = (bD > 255) ? 255 : bD;
}

bool ehVerde(int r, int g, int b) {
  // Verde claro
  if ((r < 250) && (b < 240) && (g > 240) && (g > r) && (g > b)) {
    return false;
  } 
  // Verde escuro
  else if ((r < 115) && (b < 115) && (g > 55) && (g > r) && (g > b)) {
    return true;
  }
  return false;
}

// ======== DETECÇÃO DO VERDE ESQUERDO ==========
bool DetectaverdeE() {
  return ehVerde(rE, gE, bE);
}

// ======== DETECÇÃO DO VERDE DIREITO ========
bool DetectaverdeD() {
  return ehVerde(rD, gD, bD);
}

// ======== DETECÇÃO DE AMBOS OS VERDES (RETORNO) ========
bool verdeDetectado() {
  return DetectaverdeE() && DetectaverdeD(); // Retorno se ambos detectam verde
}

// ======== DETECÇÃO DE PARADA (VERMELHO) =========
bool vermelhoDetectado() {
  int limiarVermelho = 100;
  // Verifica o sensor esquerdo
  bool vermelhoE = ((rE > gE + 10) && (rE > bE + 10) && (rE > limiarVermelho));
  //Serial.print("VermelhoE: ");
  //Serial.println(vermelhoE);
  
  // Verifica o sensor direito
  bool vermelhoD = ((rD > gD + 10) && (rD > bD + 10) && (rD > limiarVermelho));
  //Serial.print("VermelhoD: ");
  //Serial.println(vermelhoD);

  return vermelhoE || vermelhoD;
}


// =======================
// PID

void calcula_PID() {
  P = erro - setPoint;
  integral += P;
  D = P - erro_anterior;
  erro_anterior = P;

  output = (Kp * P) + (Ki * integral) + (Kd * D);

  // Limita o output para evitar valores muito altos
  output = constrain(output, -VELOCIDADE, VELOCIDADE);

  velocidade_direita = VELOCIDADE - output;
  velocidade_esquerda = VELOCIDADE + output;

  velocidade_direita = constrain(velocidade_direita, VELOCIDADE_MINIMA, VELOCIDADE_MAXIMA);
  velocidade_esquerda = constrain(velocidade_esquerda, VELOCIDADE_MINIMA, VELOCIDADE_MAXIMA);
}

// =======================
// LOOP

void loop() {
  float S1 = analogRead(SENSOR_EXTREMO_ESQUERDO) / 1023.0; // A divisão por 1023 faz com que os valores cheguem mais próximos de 0 
  float S2 = analogRead(SENSOR_ESQUERDO) / 1023.0;
  float S3 = analogRead(SENSOR_DIREITO) / 1023.0;
  float S4 = analogRead(SENSOR_EXTREMO_DIREITO) / 1023.0;
  int distance = sensor.getCM();

  atualizaLedDesligadoEsquerdo();
  //atualizaLedDesligadoDireito();
  corrigeLeituras();  // Normaliza valores para análise correta

  //verdeDetectado();
  
  if (vermelhoDetectado()) {
    //Serial.println("VERMELHO");
    analogWrite(MOTOR_ESQUERDO, 0);
    analogWrite(MOTOR_DIREITO, 0);
    digitalWrite(DIRECAO_ESQUERDA, LOW);
    digitalWrite(DIRECAO_DIREITA, LOW);

    //Serial.println("VERMELHO DETECTADO - PARANDO O ROBO");
    for (int i = 0; i < 10; i++) {
      analogWrite(ledPin, 255);
      delay(200);
      analogWrite(ledPin, 0);
      delay(200);
    }
    delay(5000);
    return;
  }

  // A partir daqui quando aparecer "> LIMIAR" o sensor está detectando a linha preta e "<= LIMIAR" significa que está retornando branco
  if ((S1 > LIMIAR) && (S2 > LIMIAR) && (S3 > LIMIAR) && (S4 > LIMIAR)) {
    analogWrite(MOTOR_ESQUERDO, 0);
    analogWrite(MOTOR_DIREITO, 0);
    //Serial.print(" 4 SENSORES DETECTARAM PRETO!");
    delay(500);

    atualizaLedDesligadoEsquerdo(),atualizaLedDesligadoDireito();

    bool verdeE = DetectaverdeE();
    bool verdeD = DetectaverdeD();

    /*int ledPinVerdeE = 12; // Pino PWM ligando o led central (pode ser qualquer pino PWM do Arduino)

    int ledPinVerdeD = 13; // Pino PWM ligando o led central (pode ser qualquer pino PWM do Arduino)

    int tempoLimiteLed = 500;
    int timer = millis();*/

    /*if (verdeE && verdeD) {
      tempoLimiteLed = 500;
      timer = millis();
      if (millis() - timer < tempoLimiteLed) {
        analogWrite(ledPinVerdeE, 255);
        analogWrite(ledPinVerdeD, 255);
      }
      else {
        analogWrite(ledPinVerdeE, 0);
        analogWrite(ledPinVerdeD, 0);
      }
    }
    else if (verdeE) {
      tempoLimiteLed = 500;
      timer = millis();
      if (millis() - timer < tempoLimiteLed) {
        analogWrite(ledPinVerdeE, 255);
      }
      else {
        analogWrite(ledPinVerdeE, 0);
      }
    }
    else if (verdeD) {
      tempoLimiteLed = 100;
      timer = millis();
      if (millis() - timer < tempoLimiteLed) {
        analogWrite(ledPinVerdeD, 255);
      }
      else {
        analogWrite(ledPinVerdeD, 0);
      }
    }
    else {
      analogWrite(ledPinVerdeE, 0);
      analogWrite(ledPinVerdeD, 0);
    }*/

    /*Serial.print("(R: "); Serial.print(rE);
    Serial.print(", G: "); Serial.print(gE);
    Serial.print(", B: "); Serial.print(bE);
    Serial.println(")");

    Serial.print("(R: "); Serial.print(rD);
    Serial.print(", G: "); Serial.print(gD);
    Serial.print(", B: "); Serial.print(bD);
    Serial.println(")");*/

    if (verdeE && verdeD) {
      //Serial.println("Verde dos dois lados – retorno!");
      giroRetorno();
      delay(2000);
    }
    else if (verdeE) {
      //Serial.println("Verde à esquerda!");
      // Pisca LED ou ação desejada
      giroCurvaFechadaEsquerdaPLUS();
      delay(2000);
    } 
    else if (verdeD) {
      //Serial.println("Verde à direita!");
      giroCurvaFechadaDireitaPLUS();
      delay(2000);
    } 
    /*if (verdeD || verdeE )
      Serial.println("Verde indentificado")
      
      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      digitalWrite(DIRECAO_ESQUERDA, LOW);
      digitalWrite(DIRECAO_DIREITA, LOW);
      delay(5000);
      
      Serial.println("RÉ");
      digitalWrite(DIRECAO_ESQUERDA, HIGH);
      digitalWrite(DIRECAO_DIREITA, HIGH);
      analogWrite(MOTOR_ESQUERDO, 10);
      analogWrite(MOTOR_DIREITO, 10);
      delay(200);

      
      if (verdeE && verdeD) {
        Serial.println("Verde dos dois lados – retorno!");
        giroRetorno();
        delay(5000);
      }
      else if (verdeE) {
        Serial.println("Verde à esquerda!");
        // Pisca LED ou ação desejada
        giroCurvaFechadaEsquerdaPLUS();
      } 
      else if (verdeD) {
        Serial.println("Verde à direita!");
        giroCurvaFechadaDireitaPLUS();
        delay(5000);
      }
      else {
        Serial.println("Siga em frente! ");
        analogWrite(MOTOR_ESQUERDO, 150);
        analogWrite(MOTOR_DIREITO, 150);
        delay(100);
      }
    return*/

    else {
      //Serial.println("Verificando...");
      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      
      //Serial.println("RÉ");
      digitalWrite(DIRECAO_ESQUERDA, HIGH);
      digitalWrite(DIRECAO_DIREITA, HIGH);
      analogWrite(MOTOR_ESQUERDO, 50);
      analogWrite(MOTOR_DIREITO, 50);
      delay(100);

      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      digitalWrite(DIRECAO_ESQUERDA, LOW);
      digitalWrite(DIRECAO_DIREITA, LOW);
      delay(5000);

      //Serial.println("Verificando...");
      atualizaLedDesligadoEsquerdo(),atualizaLedDesligadoDireito();

      verdeE = DetectaverdeE();
      verdeD = DetectaverdeD();

      if (verdeE && verdeD) {
        //Serial.println("Verde dos dois lados – retorno!");
        giroRetorno();
        delay(5000);
      }
      else if (verdeE) {
        //Serial.println("Verde à esquerda!");
        // Pisca LED ou ação desejada
        giroCurvaFechadaEsquerdaPLUS();
      } 
      else if (verdeD) {
        //Serial.println("Verde à direita!");
        giroCurvaFechadaDireitaPLUS();
        delay(5000);
      }
      else {
        //Serial.println("Siga em frente! ");
        analogWrite(MOTOR_ESQUERDO, 100);
        analogWrite(MOTOR_DIREITO, 100);
        delay(250);
      }
    }
    return;

  }

// Condicional do Obstáculo
  /*if(distance <= 0.07){
    //VELOCIDADE = 20; // Controla a velocidade do carrinho
    Desvio();
    //Serial.println("===========================AMOGUS============================= ");
    //VELOCIDADE = 40;
    return;
  }*/

  /*if ((S1 > LIMIAR) && (S2 > LIMIAR) && (S3 > LIMIAR) && (S4 > LIMIAR) || (S1 > LIMIAR) && (S2 > LIMIAR) || (S3 > LIMIAR) && (S4 > LIMIAR)) {
    analogWrite(MOTOR_ESQUERDO, 0);
    analogWrite(MOTOR_DIREITO, 0);
    Serial.print(" 4 SENSORES DETECTARAM PRETO!");
    delay(500);

    atualizaLedDesligadoEsquerdo(),atualizaLedDesligadoDireito();

    bool verdeE = DetectaverdeE();
    bool verdeD = DetectaverdeD();

    Serial.print("(R: "); Serial.print(rE);
    Serial.print(", G: "); Serial.print(gE);
    Serial.print(", B: "); Serial.print(bE);
    Serial.println(")");

    Serial.print("(R: "); Serial.print(rD);
    Serial.print(", G: "); Serial.print(gD);
    Serial.print(", B: "); Serial.print(bD);
    Serial.println(")");

    if (verdeE && verdeD) {
      Serial.println("Verde dos dois lados – retorno!");
      giroRetorno();
      delay(2000);
    }
    else if (verdeE) {
      Serial.println("Verde à esquerda!");
      // Pisca LED ou ação desejada
      giroCurvaFechadaEsquerdaPLUS();
      delay(2000);
    } 
    else if (verdeD) {
      Serial.println("Verde à direita!");
      giroCurvaFechadaDireitaPLUS();
      delay(2000);
    } 
    else {
      Serial.println("Verificando...");
      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      
      Serial.println("RÉ");
      digitalWrite(DIRECAO_ESQUERDA, HIGH);
      digitalWrite(DIRECAO_DIREITA, HIGH);
      analogWrite(MOTOR_ESQUERDO, 80);
      analogWrite(MOTOR_DIREITO, 80);
      delay(200);

      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      digitalWrite(DIRECAO_ESQUERDA, LOW);
      digitalWrite(DIRECAO_DIREITA, LOW);
      delay(1000);

      atualizaLedDesligadoEsquerdo();
      atualizaLedDesligadoDireito();

      bool verdeE = DetectaverdeE();
      bool verdeD = DetectaverdeD();

      Serial.print("(R: "); Serial.print(rE);
      Serial.print(", G: "); Serial.print(gE);
      Serial.print(", B: "); Serial.print(bE);
      Serial.println(")");

      Serial.print("(R: "); Serial.print(rD);
      Serial.print(", G: "); Serial.print(gD);
      Serial.print(", B: "); Serial.print(bD);
      Serial.println(")");

      Serial.println("Verificando...");

      if (verdeE && verdeD) {
        Serial.println("Verde dos dois lados – retorno!");
        giroRetorno();
        delay(2000);
      }
      else if (verdeE) {
        Serial.println("Verde à esquerda!");
        // Pisca LED ou ação desejada
        giroCurvaFechadaEsquerdaPLUS();
      } 
      else if (verdeD) {
        Serial.println("Verde à direita!");
        giroCurvaFechadaDireitaPLUS();
        delay(2000);
      }
      else {
        Serial.println("Siga em frente! ");
        analogWrite(MOTOR_ESQUERDO, 80);
        analogWrite(MOTOR_DIREITO, 80);
        delay(100);
      }
    }
    
    verdeE = false;
    verdeD = false;
    return;
  }*/


  /*if ((S1 > LIMIAR) && (S2 > LIMIAR) && (S3 > LIMIAR) && (S4 > LIMIAR) || (S1 > LIMIAR) && (S2 > LIMIAR) || (S3 > LIMIAR) && (S4 > LIMIAR)) {
    analogWrite(MOTOR_ESQUERDO, 0);
    analogWrite(MOTOR_DIREITO, 0);
    Serial.print(" 4 SENSORES DETECTARAM PRETO!");
    delay(500);

    atualizaLedDesligadoEsquerdo(),atualizaLedDesligadoDireito();

    bool verdeE = DetectaverdeE();
    bool verdeD = DetectaverdeD();

    Serial.print("(R: "); Serial.print(rE);
    Serial.print(", G: "); Serial.print(gE);
    Serial.print(", B: "); Serial.print(bE);
    Serial.println(")");

    Serial.print("(R: "); Serial.print(rD);
    Serial.print(", G: "); Serial.print(gD);
    Serial.print(", B: "); Serial.print(bD);
    Serial.println(")");

    if (verdeE && verdeD) {
      Serial.println("Verde dos dois lados – retorno!");
      giroRetorno();
      delay(2000);
    }
    else if (verdeE) {
      Serial.println("Verde à esquerda!");
      // Pisca LED ou ação desejada
      giroCurvaFechadaEsquerdaPLUS();
      delay(2000);
    } 
    else if (verdeD) {
      Serial.println("Verde à direita!");
      giroCurvaFechadaDireitaPLUS();
      delay(2000);
    } 
    else {
      Serial.println("Verificando...");
      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      
      Serial.println("RÉ");
      digitalWrite(DIRECAO_ESQUERDA, HIGH);
      digitalWrite(DIRECAO_DIREITA, HIGH);
      analogWrite(MOTOR_ESQUERDO, 80);
      analogWrite(MOTOR_DIREITO, 80);
      delay(200);

      analogWrite(MOTOR_ESQUERDO, 0);
      analogWrite(MOTOR_DIREITO, 0);
      digitalWrite(DIRECAO_ESQUERDA, LOW);
      digitalWrite(DIRECAO_DIREITA, LOW);
      delay(1000);

      atualizaLedDesligadoEsquerdo();
      atualizaLedDesligadoDireito();

      bool verdeE = DetectaverdeE();
      bool verdeD = DetectaverdeD();

      Serial.print("(R: "); Serial.print(rE);
      Serial.print(", G: "); Serial.print(gE);
      Serial.print(", B: "); Serial.print(bE);
      Serial.println(")");

      Serial.print("(R: "); Serial.print(rD);
      Serial.print(", G: "); Serial.print(gD);
      Serial.print(", B: "); Serial.print(bD);
      Serial.println(")");

      Serial.println("Verificando...");

      if (verdeE && verdeD) {
        Serial.println("Verde dos dois lados – retorno!");
        giroRetorno();
        delay(2000);
      }
      else if (verdeE) {
        Serial.println("Verde à esquerda!");
        // Pisca LED ou ação desejada
        giroCurvaFechadaEsquerdaPLUS();
      } 
      else if (verdeD) {
        Serial.println("Verde à direita!");
        giroCurvaFechadaDireitaPLUS();
        delay(2000);
      }
      else {
        Serial.println("Siga em frente! ");
        analogWrite(MOTOR_ESQUERDO, 80);
        analogWrite(MOTOR_DIREITO, 80);
        delay(100);
      }
    }
    
    verdeE = false;
    verdeD = false;
    return;
  }*/


  if ((S1 > LIMIAR) && (S2 <= LIMIAR) && (S3 <= LIMIAR) && (S4 > LIMIAR)) { // Esta parte -> Se os sensores extremos identificarem preto o carrinho anda para frente por um breve momento
    analogWrite(MOTOR_ESQUERDO, 150);
    analogWrite(MOTOR_DIREITO, 150);
    return;
  }

  if ((S1 > LIMIAR && S2 <= LIMIAR && S3 <= LIMIAR && S4 <= LIMIAR) || (S1 > LIMIAR && S2 > LIMIAR && S3 <= LIMIAR && S4 <= LIMIAR)) { // Esta parte -> Caso o senso externo esquerdo identifique a linha, fará um giro de 90 graus
    if (!aguardandoGiroEsquerda) {
      tempoInicioEsquerda = millis();
      aguardandoGiroEsquerda = true;
    } else if (millis() - tempoInicioEsquerda >= 0) { // Aqui ele verifica se o tempo de leitura do sensor para iniciar o giro 
      giroCurvaFechadaEsquerda(); // Executa a função de curva fechada
      integral = 0;
      erro_anterior = 0;
      tempoInicioEsquerda = 0;
      aguardandoGiroEsquerda = false; // Reseta o PID e a verificação do sensor
      return;
    }
  } else {
    aguardandoGiroEsquerda = false; // Reseta a verificação por precaução e não executa o giro
  }

  if ((S1 <= LIMIAR && S2 <= LIMIAR && S3 <= LIMIAR && S4 > LIMIAR) || (S1 <= LIMIAR && S2 <= LIMIAR && S3 > LIMIAR && S4 > LIMIAR)) {
    if (!aguardandoGiroDireita) {
      tempoInicioDireita = millis();
      aguardandoGiroDireita = true;
    } else if (millis() - tempoInicioDireita >= 0) { // Aqui ele verifica se o tempo de leitura do sensor para iniciar o giro
      giroCurvaFechadaDireita();
      integral = 0;
      erro_anterior = 0;
      tempoInicioDireita = 0;
      aguardandoGiroDireita = false;
      return;
    }
  } else {
    aguardandoGiroDireita = false;
  }

  erro = S2 - S3;
  calcula_PID();

  analogWrite(MOTOR_ESQUERDO, velocidade_esquerda);
  analogWrite(MOTOR_DIREITO, velocidade_direita);

  // Atualiza sensores de cor sem bloquear
  //atualizaLedDesligadoEsquerdo();
  //atualizaLedDesligadoDireito();

  if (leituraProntaEsquerdo && leituraProntaDireito) {
    // Corrige as leituras e normaliza
    //corrigeLeituras();

    // Novos limiares refinados
    int brancoMin = 235;
    int brancoMax = 255;
    int limiarVerde = 100;
    int limiarVermelho = 100;

    // Calibração mais precisa

    bool verdeEsquerdo = false;

    if ((rE < 235) && (bE < 235) && (gE > rE + 0) && (gE > bE + 0) && (gE > 240)) {
      verdeEsquerdo = true;
    } 
    else if ((rE < 100) && (gE < 100) && (bE < 100) && (gE > rE) && (gE > bE) && (gE > 55)) {
      verdeEsquerdo = true;
    }
    else {
      verdeEsquerdo = false;
    }


    bool verdeDireito = false;

    if ((rD < 240) && (bD < 240) && (gD > rD + 0) && (gD > bD + 0) && (gD > 240)) {
      // Verde claro / brilhante
      verdeDireito = true;
    } 
    else if ((rD < 100) && (gD < 100) && (bD < 100) && (gD > rD) && (gD > bD) && (gD > 55)) {
      // Verde escuro
      verdeDireito = true;
    }
    else {
      // Não é verde
      verdeDireito = false;
    }


    bool vermelhoEsquerdo = (rE > gE + 15) && (rE > bE + 15) && (rE > limiarVermelho);
    bool vermelhoDireito  = (rD > gD + 15) && (rD > bD + 15) && (rD > limiarVermelho);

 
    // Reseta flags para próxima leitura
    leituraProntaEsquerdo = false;
    leituraProntaDireito = false;
  }

  /*Serial.print("SENSOR 1 ");
  Serial.println(S1);
  Serial.print(" ");
  Serial.print("SENSOR 2 ");
  Serial.print(S2);
  Serial.print(" ");
  Serial.print("SENSOR 3 ");
  Serial.println(S3);
  Serial.print(" ");
  Serial.print("SENSOR 4 ");
  Serial.print(S4);
  Serial.print(" ");*/

  // Write values to serial port
  /*Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println("cm");*/

  // Exibe os valores corrigidos para diagnóstico
  /*Serial.print(" | RGB E: ");
  Serial.print(rE); Serial.print(", ");
  Serial.print(gE); Serial.print(", ");
  Serial.print(bE);

  Serial.print(" | RGB D: ");
  Serial.print(rD); Serial.print(", ");
  Serial.print(gD); Serial.print(", ");
  Serial.print(bD);*/

}