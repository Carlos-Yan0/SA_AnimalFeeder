#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "index.h"
#include <ArduinoJson.h>
#include "time.h"


// ==============================
// === CONFIGURAÇÕES DE PINOS ===
// ==============================
const uint8_t PIN_TRIG    = 20;
const uint8_t PIN_ECHO    = 21;
const uint8_t PIN_BUZZER  = 22;
const uint8_t PIN_SERVO   = 23;
Servo servo;

// ==============================
// === NTP PARA HORARIO ATUAL ===
// ==============================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;  // GMT -3
short int ultimoMinutoExecutado[4] = { -1, -1, -1, -1 };

void printLocalDateTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Não foi possível obter o tempo.");
    return;
  }

  char timeStringBuff[64];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeInfo);
  Serial.println(timeStringBuff);
}

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

// ===============
// ==== WI-FI ====
// ===============

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
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, 0, ntpServer);
  printLocalDateTime();
}

// ======================================================
// ===   CONFIGURAÇÕES DAS AÇÕES DE CADA COMPONENTE   ===
// ======================================================

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


void despejarRacao () {
    Serial.print("Despejando ração. Nível de despejo: ");
    Serial.println(sConfig.nivel_despejo);

    somChamarAtencao();
    servo.write(170);
    delay(sConfig.nivel_despejo*1000);
    servo.write(0);
}


void somChamarAtencao() {
    for (uint8_t i = 0; i < 2; i++){
      tone(PIN_BUZZER, 300);
      delay(100);
      noTone(PIN_BUZZER);
      delay(100);

      tone(PIN_BUZZER, 500);
      delay(100);
      noTone(PIN_BUZZER);
      delay(100);

      tone(PIN_BUZZER, 600);
      delay(100);
      noTone(PIN_BUZZER);
      delay(100);

      tone(PIN_BUZZER, 400);
      delay(100);
      noTone(PIN_BUZZER);
      delay(100);

      delay(800);
    }
    delay(1200);
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


void handleDespejar() {
    if (!sConfig.nivel_despejo) {
        server.send(500, "text/plain", "É necessário selecionar uma opção antes do despejo imediato.");
        return;
    }

    despejarRacao();
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

    for (int i = 0; i < 4; i++) {
        rotinas[i].hora   = doc["rotina"][i][0];
        rotinas[i].minuto = doc["rotina"][i][1];
        rotinas[i].ativo  = doc["rotina"][i][2];

        sConfig.rotinas[i] = rotinas[i];
    }

    server.send(200, "text/plain", "Ok!");
}


void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/despejar", handleDespejar);
  server.on("/salvar", handleJSON);
  server.onNotFound(handleNotFound);
}

// ==============
// === ROTINA ===
// ==============

void verificarRotinas() {
    struct tm timeInfo;
    getLocalTime(&timeInfo);

    int horaAtual = timeInfo.tm_hour;
    int minutoAtual = timeInfo.tm_min;

    for (int i = 0; i < 4; i++) {
        Rotina r = sConfig.rotinas[i];

        if (!r.ativo) continue;

        if (r.hora == horaAtual && r.minuto == minutoAtual) {

            if (ultimoMinutoExecutado[i] == minutoAtual) continue;

            Serial.printf("Executando rotina %d\n", i);
            despejarRacao();

            ultimoMinutoExecutado[i] = minutoAtual;
        }
    }
}

// =============
// === SETUP ===
// =============

void setup() {
  Serial.begin(115200);

  servo.attach(PIN_SERVO, 544, 2400);
  servo.write(0);

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
  verificarRotinas();
  delay(10);
}
