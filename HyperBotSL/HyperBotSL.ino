// =======================
// Pinos dos motores
#define MOTOR_ESQUERDO 11
#define MOTOR_DIREITO 9
#define DIRECAO_ESQUERDA 10
#define DIRECAO_DIREITA 8


// Pinos dos sensores digitais (linha)
#define SENSOR_EXTREMO_ESQUERDO 4
#define SENSOR_ESQUERDO 5
#define SENSOR_DIREITO 6
#define SENSOR_EXTREMO_DIREITO 7


// Parâmetros PID
float Kp = 170;
float Ki = 0;
float Kd = 10;


float setPoint = 0;
float erro = 0;
float erro_anterior = 0;
float integral = 0;
float P = 0, D = 0, output = 0;


// Controle dos motores
int VELOCIDADE = 80; // Velocidade base
int VELOCIDADE_MAXIMA = 255;
int VELOCIDADE_MINIMA = 0;


int velocidade_esquerda = 0;
int velocidade_direita = 0;

// Variáveis usadas para calcular o tempo de leitura dos sensores
unsigned long tempoInicioEsquerda = 0;
bool aguardandoGiroEsquerda = false;

unsigned long tempoInicioDireita = 0;
bool aguardandoGiroDireita = false;

// =======================
void setup() {
  pinMode(MOTOR_ESQUERDO, OUTPUT);
  pinMode(MOTOR_DIREITO, OUTPUT);
  pinMode(DIRECAO_ESQUERDA, OUTPUT);
  pinMode(DIRECAO_DIREITA, OUTPUT);

  pinMode(SENSOR_EXTREMO_ESQUERDO, INPUT);
  pinMode(SENSOR_ESQUERDO, INPUT);
  pinMode(SENSOR_DIREITO, INPUT);
  pinMode(SENSOR_EXTREMO_DIREITO, INPUT);

  Serial.begin(9600); // <-- faltava
}


void calcula_PID() {
  P = erro - setPoint;
  integral += P;
  D = P - erro_anterior;
  erro_anterior = P;


  output = (Kp * P) + (Ki * integral) + (Kd * D);


  // Limita o output para evitar valores muito altos
  output = constrain(output, -VELOCIDADE, VELOCIDADE);


  velocidade_direita = VELOCIDADE + output;
  velocidade_esquerda = VELOCIDADE - output;


  velocidade_direita = constrain(velocidade_direita, VELOCIDADE_MINIMA, VELOCIDADE_MAXIMA);
  velocidade_esquerda = constrain(velocidade_esquerda, VELOCIDADE_MINIMA, VELOCIDADE_MAXIMA);
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

    float sensorE = analogRead(SENSOR_ESQUERDO);
    float sensorD = analogRead(SENSOR_DIREITO);

    if (passouTempoMinimo || (sensorD > 1)) break;
  }

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
}

void giroCurvaFechadaDireita() {
  const unsigned long tempoMinimo = 300;
  unsigned long inicio = millis();
  bool passouTempoMinimo = false;

  digitalWrite(DIRECAO_ESQUERDA, HIGH);
  digitalWrite(DIRECAO_DIREITA, LOW);
  analogWrite(MOTOR_ESQUERDO, 150);
  analogWrite(MOTOR_DIREITO, 0);
  delay(1);

  while (true) {
    if (millis() - inicio >= tempoMinimo) passouTempoMinimo = true;

    float sensorE = analogRead(SENSOR_ESQUERDO);
    float sensorD = analogRead(SENSOR_DIREITO);

    if (passouTempoMinimo || (sensorE > 1)) break;
  }

  analogWrite(MOTOR_ESQUERDO, 0);
  analogWrite(MOTOR_DIREITO, 0);
  digitalWrite(DIRECAO_ESQUERDA, LOW);
  digitalWrite(DIRECAO_DIREITA, LOW);
}


// =======================
void loop() {
  // Leitura dos sensores invertida (0 = branco, 1 = preto)
  int s1 = digitalRead(SENSOR_EXTREMO_ESQUERDO);
  int s2 = digitalRead(SENSOR_ESQUERDO);
  int s3 = digitalRead(SENSOR_DIREITO);
  int s4 = digitalRead(SENSOR_EXTREMO_DIREITO);

  if ((s1 == 1 && s2 == 0 && s3 == 0 && s4 == 0) || (s1 == 1 && s2 == 1 && s3 == 0 && s4 == 0)) { // Esta parte -> Caso o senso externo esquerdo identifique a linha, fará um giro de 90 graus
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

  if ((s1 == 0 && s2 == 0 && s3 == 0 && s4 == 1) || (s1 == 0 && s2 == 0 && s3 == 1 && s4 == 1)) {
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


  // Calcular erro
  erro = s2 - s3;

  // Calcula PID
  calcula_PID();


  // Aplica velocidade nos motores
  analogWrite(MOTOR_ESQUERDO, velocidade_esquerda);
  analogWrite(MOTOR_DIREITO, velocidade_direita);


  // Log
  Serial.print("s1:"); Serial.print(s1);
  Serial.print(" s2:"); Serial.print(s2);
  Serial.print(" s3:"); Serial.print(s3);
  Serial.print(" s4:"); Serial.print(s4);
  Serial.print(" | Erro:"); Serial.print(erro);
  Serial.print(" | Out:"); Serial.println(output);
}


// =======================