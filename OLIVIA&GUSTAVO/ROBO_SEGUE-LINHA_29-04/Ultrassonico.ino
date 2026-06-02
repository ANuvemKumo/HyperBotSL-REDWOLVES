float lerDistanciaCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(20);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 4000);
  if (duration == 0) return -1;
  const float distancia = (duration * 0.0343) / 2.0;

  return distancia; //Conversão para cm
}

extern void esquerda(int tempo = 0);
extern void frente(int tempo = 0);
extern void tras(int tempo = 0);
extern void direita(int tempo = 0);
extern void parar(int tempo = 0);

bool encontrouLINHA() {

  qtr.read(sensorValues);

  for (int i = 0; i < 8; i++) {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }
  Serial.println();

  for (int i = 0; i < 8; i++) {
    if (sensorValues[i] > 800) {
      return true;
    }
  }

  return false;
}


void desviarObstaculoDireita() {
  bool linha = true;
  tras(500);
  esquerda(1000);
  frente(1800);
  while (linha) {
    esquerda();
    qtr.read(sensorValues);
    if (// Pode ser otimizado usando for loop
      sensorValues[0] > LIMIAR_PRETO || 
      sensorValues[1] > LIMIAR_PRETO ||
      sensorValues[2] > LIMIAR_PRETO ||
      sensorValues[3] > LIMIAR_PRETO ||
      sensorValues[4] > LIMIAR_PRETO ||
      sensorValues[5] > LIMIAR_PRETO ||
      sensorValues[6] > LIMIAR_PRETO ||
      sensorValues[7] > LIMIAR_PRETO
    )
    {
      linha = false;
    }
  }
}

void desviarObstaculoEsquerda() {
  Serial.println("Contornando pela Esquerda!");
  tras(500);
  direita(800);
  frente(1800);

  unsigned long inicioBusca = millis();

  while (true) {
    Serial.println("PROCURANDO...");
    esquerda_2();
    if (encontrouLINHA()){
      Serial.println("ENCONTROU A LINHA PRETA!");
      parar(500);
      frente(200);
      direita(500);
      parar(500);
      break;
    }
  }
}

void detectarObstaculo() {
  float dist = lerDistanciaCm();
  //Serial.println("Distancia: ");
  //Serial.println(dist);
  if (dist > 0 && dist <= limiteCm) {
    Serial.println("OBSTACULO");
    parar(2000);
    if (D_direita){
      desviarObstaculoDireita();
    }
    else if (D_esquerda){
      desviarObstaculoEsquerda();
    }
    else{
      esquerda();
      direita();
      parar();
    }
  }
}


