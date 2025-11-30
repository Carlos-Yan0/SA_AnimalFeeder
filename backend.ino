#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "index.h"
#include <ArduinoJson.h>


// ==============================
// === CONFIGURAÇÕES DE PINOS ===
// ==============================
const uint8_t PIN_TRIG    = 20;
const uint8_t PIN_ECHO    = 21;
const uint8_t PIN_BUZZER  = 22;
const uint8_t PIN_SERVO   = 23;

// =====================================
// === STRUCTS DE CONFIGS DO SISTEMA ===
// =====================================
struct Rotina {
    unsigned short int hora;
    unsigned short int minuto;
    bool ativo;
};

struct Config {
    unsigned short int nivel_despejo;
    Rotina rotina[4];
};

struct Rotina rotinas[4];
struct Config sConfig;

// ===========================
// ==== CREDENCIAIS WI-FI ====
// ===========================

const char* SSID     = "ESP";
const char* PASSWORD = "123456esp";

WebServer server(80);

void configurarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

  WiFi.disconnect(true);
  delay(1000);

  Serial.print("\nConectando ao WiFi...");
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =============================
// ===   MÓDULO: LED/BOTÃO   ===
// =============================

long medirDistancia() {
  // Gera pulso no TRIG
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Lê o pulso do ECHO
  long duracao = pulseIn(PIN_ECHO, HIGH);

  // Distância em centímetros
  long distancia = duracao * 0.034 / 2;

  return distancia;
}

void despejoLeve() {
  Serial.println("Despejando pouca ração...");
  servo.write(80);
  delay(3000);
  servo.write(0);

  server.send(200, "text/plain", "despejo Leve Ok");
  Serial.println("Animal Alimentado!");
}

void despejoMedio() {
  Serial.println("Despejando uma quantidade consideravel de ração...");
  servo.write(80);
  delay(5000);
  servo.write(0);

  server.send(200, "text/plain", "despejo medio Ok");
  Serial.println("Animal Alimentado!");
}

void despejoGrande() {
  Serial.println("Despejando Muita ração...");
  servo.write(80);
  chamarAtencao();
  delay(8000);
  servo.write(0);

  server.send(200, "text/plain", "despejo grande Ok");
  Serial.println("Animal Alimentado!");
}

void despejoPersonalizado(int tempo){
  Serial.println("Despejando uma quantidade personalizada de ração...");
  servo.write(80);
  chamarAtencao();
  delay(tempo * 1000);
  servo.write(0);

  server.send(200, "text/plain", "despejo personalizado Ok");
  Serial.println("Animal Alimentado!");
}

void chamarAtencao() {
  // FALTA O SOM DO BUZZER
}

void handleStatus() {
  server.send(200, "text/plain");
}


// ===============================
// ===   MÓDULO: PÁGINAS WEB   ===
// ===============================

void handleRoot() {
  Serial.println("Acessando página principal");
  server.send(200, "text/html", index_html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void handleJSON() {
    println("Recebendo JSON via POST");

    String jsonString = server.arg("plain");
    println(jsonString);

    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, jsonString);

    if (erro) {
        Serial.print("Falha ao ler JSON: ");
        Serial.println(erro.f_str());
        server.send(500, "text/plain", "Erro ao processar JSON");
        return;
    }
    
    sConfig.nivel_despejo = doc["nivelDespejo"];
    
    rotinas[0].hora = doc["rotina"][0][0];
    rotinas[0].minuto = doc["rotina"][0][1];
    rotinas[0].ativo = doc["rotina"][0][2];
    
    rotinas[1].hora = doc["rotina"][1][0];
    rotinas[1].minuto = doc["rotina"][1][1];
    rotinas[1].ativo = doc["rotina"][1][2];
    
    rotinas[2].hora = doc["rotina"][2][0];
    rotinas[2].minuto = doc["rotina"][2][1];
    rotinas[2].ativo = doc["rotina"][2][2];
    
    rotinas[3].hora = doc["rotina"][3][0];
    rotinas[3].minuto = doc["rotina"][3][1];
    rotinas[3].ativo = doc["rotina"][3][2];
    
    sConfig.rotinas[0] = rotinas[0];
    sConfig.rotinas[1] = rotinas[1];
    sConfig.rotinas[2] = rotinas[2];
    sConfig.rotinas[3] = rotinas[3];
}

void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/despejoLeve", despejoLeve);
  server.on("/despejoMedio", despejoMedio);
  server.on("/despejoGrande", despejoGrande);
  server.on("/despejoPersonalizado", despejoPersonalizado);
  server.on("/status", handleStatus);
  server.on("/salvar", handleJSON);
  server.onNotFound(handleNotFound);
}


void rotinaAutomatica() {
  automacaoHoraria();
}

// =============
// === SETUP ===
// =============

void setup() {
  Serial.begin(115200);

  Servo servo;
  servo.attach(PIN_SERVO, 500, 2400);

  configurarWiFi();
  delay(2000);

  configurarRotas();
  server.begin();

  Serial.println("Servidor Web iniciado!");
}

// ============
// === LOOP ===
// ============

void loop() {
  server.handleClient();
  delay(10);
}
