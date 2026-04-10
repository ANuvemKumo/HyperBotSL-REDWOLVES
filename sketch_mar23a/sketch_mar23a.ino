#include <WiFi.h>
#include <WebServer.h>
#include <RoboCore_Vespa.h>
#include "Adafruit_TCS34725.h"
#include <Wire.h>

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