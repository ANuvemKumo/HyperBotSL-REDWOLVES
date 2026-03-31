#include <WiFi.h>
#include <WebServer.h>
#include <RoboCore_Vespa.h>
#include "Adafruit_TCS34725.h"

// ================= WIFI =================
const char* ssid = "EDUC_CE208";
const char* password = "ac2ce0ss8@educ";

WebServer server(80);

// ================= MOTORES =================
VespaMotors motores;

int VELOCIDADE_BASE = 65; //90
int VELOCIDADE_MAX = 70;
int LIMITE_CONTROLE = 65;

// ================= SENSORES =================
#define EXT_DIR 32
#define DIR     33
#define ESQ     25
#define EXT_ESQ 26

int extEsq, esq, dir, extDir;

// ================= PID =================
float Kp = 25.0; //45
float Ki = 0.0;
float Kd = 15.0;

float erro = 0;
float erroAnterior = 0;
float integral = 0;
float derivada = 0;
float controle = 0;

// ======================== SENSOR RGB =================

#define TCAADR 0x70

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

// ================= HTML (AVANÇADO) =================
String paginaHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial; text-align: center; }
.slider { width: 80%; }
</style>
</head>

<body>
<h2>Controle PID - ESP32</h2>

<p>Kp: <span id="kpVal">25</span></p>
<input type="range" min="0" max="100" step="0.1" value="25" class="slider" id="kp">

<p>Ki: <span id="kiVal">0</span></p>
<input type="range" min="0" max="10" step="0.01" value="0" class="slider" id="ki">

<p>Kd: <span id="kdVal">15</span></p>
<input type="range" min="0" max="100" step="0.1" value="15" class="slider" id="kd">

<p>Vel Base: <span id="vbVal">70</span></p>
<input type="range" min="0" max="100" step="1" value="70" class="slider" id="vb">

<h3>Dados em tempo real</h3>

<p>Erro: <span id="erro">0</span></p>
<p>Controle: <span id="controle">0</span></p>

<h3>Sensores</h3>
<p>EXT_ESQ: <span id="extEsq">0</span></p>
<p>ESQ: <span id="esq">0</span></p>
<p>DIR: <span id="dir">0</span></p>
<p>EXT_DIR: <span id="extDir">0</span></p>

<script>
function sendData() {
  let kp = document.getElementById("kp").value;
  let ki = document.getElementById("ki").value;
  let kd = document.getElementById("kd").value;
  let vb = document.getElementById("vb").value;

  document.getElementById("kpVal").innerText = kp;
  document.getElementById("kiVal").innerText = ki;
  document.getElementById("kdVal").innerText = kd;
  document.getElementById("vbVal").innerText = vb;

  fetch(`/set?kp=${kp}&ki=${ki}&kd=${kd}&vb=${vb}`);
}

// Atualiza quando mexe nos sliders
document.querySelectorAll(".slider").forEach(slider => {
  slider.addEventListener("input", sendData);
});

// Atualiza dados do robô
setInterval(() => {
  fetch("/data")
    .then(res => res.json())
    .then(data => {
      document.getElementById("erro").innerText = data.erro;
      document.getElementById("controle").innerText = data.controle;

      document.getElementById("extEsq").innerText = data.extEsq;
      document.getElementById("esq").innerText = data.esq;
      document.getElementById("dir").innerText = data.dir;
      document.getElementById("extDir").innerText = data.extDir;
    });
}, 100);

</script>
</body>
</html>
)rawliteral";
}

// ================= ROTAS =================
void handleRoot() {
  server.send(200, "text/html", paginaHTML());
}

void handleSet() {
  if (server.hasArg("kp")) Kp = server.arg("kp").toFloat();
  if (server.hasArg("ki")) Ki = server.arg("ki").toFloat();
  if (server.hasArg("kd")) Kd = server.arg("kd").toFloat();
  if (server.hasArg("vb")) VELOCIDADE_BASE = server.arg("vb").toInt();

  server.send(200, "text/plain", "OK");
}

void handleData() {
  String json = "{";
  json += "\"erro\":" + String(erro) + ",";
  json += "\"controle\":" + String(controle) + ",";
  json += "\"extEsq\":" + String(extEsq) + ",";
  json += "\"esq\":" + String(esq) + ",";
  json += "\"dir\":" + String(dir) + ",";
  json += "\"extDir\":" + String(extDir);
  json += "}";

  server.send(200, "application/json", json);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(EXT_ESQ, INPUT);
  pinMode(ESQ, INPUT);
  pinMode(DIR, INPUT);
  pinMode(EXT_DIR, INPUT);

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

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/data", handleData);
  server.begin();
}

// ================= LOOP =================
void loop() {

  server.handleClient();

  extEsq = digitalRead(EXT_ESQ);
  esq    = digitalRead(ESQ);
  dir    = digitalRead(DIR);
  extDir = digitalRead(EXT_DIR);

  if (extEsq == 1 && esq == 0 && dir == 0 && extDir == 0) erro = -3;
  else if (extEsq == 1 && esq == 1 && dir == 0 && extDir == 0) erro = -2;
  else if (extEsq == 0 && esq == 1 && dir == 0 && extDir == 0) erro = -1;
  else if (extEsq == 0 && esq == 1 && dir == 1 && extDir == 0) erro = 0;
  else if (extEsq == 0 && esq == 0 && dir == 1 && extDir == 0) erro = 1;
  else if (extEsq == 0 && esq == 0 && dir == 1 && extDir == 1) erro = 2;
  else if (extEsq == 0 && esq == 0 && dir == 0 && extDir == 1) erro = 3;
  else erro = 0;

  integral += erro;
  derivada = erro - erroAnterior;

  controle = (Kp * erro) + (Ki * integral) + (Kd * derivada);
  controle = constrain(controle, -LIMITE_CONTROLE, LIMITE_CONTROLE);

  erroAnterior = erro;

  int velocidadeEsq = VELOCIDADE_BASE + controle;
  int velocidadeDir = VELOCIDADE_BASE - controle;

  velocidadeEsq = constrain(velocidadeEsq, -VELOCIDADE_MAX, VELOCIDADE_MAX);
  velocidadeDir = constrain(velocidadeDir, -VELOCIDADE_MAX, VELOCIDADE_MAX);

  motores.setSpeedLeft(velocidadeEsq);
  motores.setSpeedRight(velocidadeDir);

  delay(10);
}