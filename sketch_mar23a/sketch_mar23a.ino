/*******************************************************************************
* Vespa - Primeiros Passos para Controle de Motores DC (v1.0)
* 
* Codigo de demonstracao das funcoes de controle para o acionamento conjunto e
* individual de motores DC pela Vespa.
* 
* Copyright 2022 RoboCore.
* Escrito por Giovanni de Castro (10/06/2022).
* 
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version (<https://www.gnu.org/licenses/>).
*******************************************************************************/

//Inclusao da biblioteca da Vespa
#include <RoboCore_Vespa.h>

//Criacao do objeto "motores" para o acionamento dos motores
VespaMotors motores;

//Declaracao das variaveis de velocidade maxima e de curva
const int VELOCIDADE_MAXIMA = 100;
const int VELOCIDADE_CURVA = 50;

//Declaracao das variaveis de pausa para alteracao de estado dos motores e aceleracao
const int TEMPO_PAUSA = 1000;
const int TEMPO_ACELERACAO = 10;

//------------------------------------------------------------------------------

void setup() {

  //Inicializacao do monitor serial
  Serial.begin(115200);
  Serial.println("<--- Primeiros Passos com o Controle de Motores DC da Vespa --->");

}

//------------------------------------------------------------------------------

void loop() {

  //Funcoes de controle conjunto dos motores
  Serial.println("<--- CONTROLE CONJUNTO DOS MOTORES --->");
  Serial.println("Acionando motores para frente...");
  //Acionamento dos motores para frente
  motores.forward(VELOCIDADE_MAXIMA);
  delay(TEMPO_PAUSA); //Mantem os motores girando
  Serial.println("Parando motores...");
  //Acionamento dos motores para pararem
  motores.stop();
  delay(TEMPO_PAUSA); //Mantem os motores parados

  Serial.println("Acionando motores para tras...");
  //Acionamento dos motores para tras
  motores.backward(VELOCIDADE_MAXIMA);
  delay(TEMPO_PAUSA); //Mantem os motores girando
  Serial.println("Parando motores...");
  //Acionamento dos motores para pararem
  motores.stop();
  delay(TEMPO_PAUSA); //Mantem os motores parados

  Serial.println("Acionando motores para curva a esquerda de frente...");
  //Acionamento dos motores para curva para a esquerda indo para frente
  motores.turn(VELOCIDADE_CURVA, VELOCIDADE_MAXIMA);
  delay(TEMPO_PAUSA); //Mantem os motores girando
  Serial.println("Parando motores...");
  //Acionamento dos motores para pararem
  motores.stop();
  delay(TEMPO_PAUSA); //Mantem os motores parados

  Serial.println("Acionando motores para curva a direita de frente...");
  //Acionamento dos motores para curva para a direita indo para frente
  motores.turn(VELOCIDADE_MAXIMA, VELOCIDADE_CURVA);
  delay(TEMPO_PAUSA); //Mantem os motores girando
  Serial.println("Parando motores...");
  //Acionamento dos motores para pararem
  motores.stop();
  delay(TEMPO_PAUSA); //Mantem os motores parados

  Serial.println("Acionando motores para curva a esquerda de tras...");
  //Acionamento dos motores para curva para a esquerda indo para tras
  motores.turn(-VELOCIDADE_CURVA, -VELOCIDADE_MAXIMA);
  delay(TEMPO_PAUSA); //Mantem os motores girando
  Serial.println("Parando motores...");
  //Acionamento dos motores para pararem
  motores.stop();
  delay(TEMPO_PAUSA); //Mantem os motores parados

  Serial.println("Acionando motores para curva a direita de tras...");
  //Acionamento dos motores para curva para a direita indo para tras
  motores.turn(-VELOCIDADE_MAXIMA, -VELOCIDADE_CURVA);
  delay(TEMPO_PAUSA); //Mantem os motores girando
  Serial.println("Parando motores...");
  //Acionamento dos motores para pararem
  motores.stop();
  delay(TEMPO_PAUSA); //Mantem os motores parados

  //Funcoes de controle individual dos motores
  Serial.println("<--- CONTROLE INDIVIDUAL DOS MOTORES --->");
  Serial.println("Acelerando motor esquerdo para frente...");
  //Rampa de aceleracao para frente
  for (int i = 0; i <= VELOCIDADE_MAXIMA; i++){
    //Aciona o motor esquerdo com o valor da variavel "i"
    motores.setSpeedLeft(i);
    Serial.print("Velocidade: ");
    Serial.println(i);
    delay(TEMPO_ACELERACAO); //Atrasa a aceleracao
  }

  delay(TEMPO_PAUSA); //Mantem o motor girando

  Serial.println("Freando motor esquerdo...");
  //Rampa de desaceleracao
  for (int i = VELOCIDADE_MAXIMA; i >= 0; i--){
    //Aciona o motor esquerdo com o valor da variavel "i"
    motores.setSpeedLeft(i);
    Serial.print("Velocidade: ");
    Serial.println(i);
    delay(TEMPO_ACELERACAO); //Atrasa a desaceleracao
  }

  Serial.println("Acelerando motor direito para tras...");
  //Rampa de aceleracao para tras
  for (int i = 0; i <= VELOCIDADE_MAXIMA; i++){
    //Aciona o motor direito com o valor negativo da variavel "i"
    motores.setSpeedRight(-i);
    Serial.print("Velocidade: -");
    Serial.println(i);
    delay(TEMPO_ACELERACAO); //Atrasa a aceleracao
  }

  delay(TEMPO_PAUSA); //Mantem o motor girando

  Serial.println("Freando motor direito...");
  //Rampa de desaceleracao
  for (int i = VELOCIDADE_MAXIMA; i >= 0; i--){
    //Aciona o motor direito com o valor negativo da variavel "i"
    motores.setSpeedRight(-i);
    Serial.print("Velocidade: -");
    Serial.println(i);
    delay(TEMPO_ACELERACAO); //Atrasa a desaceleracao
  }

  delay(TEMPO_PAUSA*2); //Aguarda para repeticao das funcoes

}

//------------------------------------------------------------------------------