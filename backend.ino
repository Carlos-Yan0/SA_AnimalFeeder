#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "index.h"
#include <ArduinoJson.h>
#include "time.h"


// ==============================
// === CONFIGURAÇÕES DE PINOS ===
// ==============================
const uint8_t PIN_TRIG    = 5;
const uint8_t PIN_ECHO    = 6;
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
    Rotina rotinas[4];
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
  Serial.print("Duração:");
  Serial.println(duracao);

  // Distância em centímetros
  long distancia = duracao / 58;
  Serial.print("Distância:");
  Serial.println(distancia);

  return distancia;
}


void despejarRacao () {
  if(!sConfig.nivel_despejo || sConfig.nivel_despejo == 0){
    Serial.println("Despejo negado, insira um nivel de despejo");
  }
  else{
    Serial.print("Despejando ração. Nível de despejo: ");
    Serial.println(sConfig.nivel_despejo);

    somChamarAtencao();
    servo.write(170);
    delay(sConfig.nivel_despejo*1000);
    servo.write(0);
  }
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


String pegarProximaRotina() {
    struct tm timeInfo;
    getLocalTime(&timeInfo);

    int horaAtual = timeInfo.tm_hour;
    int minutoAtual = timeInfo.tm_min;

    int menorDiferenca = 1440; // minutos em 24h
    String proxima = "Nenhuma programada";

    for (int i = 0; i < 4; i++) {
        Rotina r = sConfig.rotinas[i];
        if (!r.ativo) continue;

        int minutoAtualTotal = horaAtual * 60 + minutoAtual;
        int minutoRotinaTotal = r.hora * 60 + r.minuto;

        int diff = minutoRotinaTotal - minutoAtualTotal;
        if (diff < 0) diff += 1440;

        if (diff < menorDiferenca) {
            menorDiferenca = diff;

            char buffer[6];
            sprintf(buffer, "%02d:%02d", r.hora, r.minuto);
            proxima = buffer;
        }
    }

    return proxima;
}


void handleStatus() {
    StaticJsonDocument<300> doc;

    doc["distancia"] = medirDistancia();
    doc["nivelDespejo"] = sConfig.nivel_despejo;

    // Envia a próxima alimentação
    doc["proximaRefeicao"] = pegarProximaRotina();

    JsonArray rot = doc.createNestedArray("rotinas");
    for (int i = 0; i < 4; i++) {
        JsonObject obj = rot.createNestedObject();
        obj["hora"]   = sConfig.rotinas[i].hora;
        obj["minuto"] = sConfig.rotinas[i].minuto;
        obj["ativo"]  = sConfig.rotinas[i].ativo;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);

    server.send(200, "application/json", jsonStr);
}


void handleDespejoManual(){
    Serial.println("Despejo manual solicitado...");
    despejarRacao();
}


void handleJSON() {
    Serial.println("Recebendo JSON via POST");

    String jsonString = server.arg("plain");
    Serial.println(jsonString);

    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, jsonString);

    if (erro) {
        Serial.print("Falha ao ler JSON: ");
        Serial.println(erro.f_str());
        server.send(500, "text/plain", "Erro ao processar JSON");
        return;
    }

    if (doc.containsKey("nivelDespejo")) {
        sConfig.nivel_despejo = doc["nivelDespejo"];
        Serial.print("Nível de despejo atualizado: ");
        Serial.println(sConfig.nivel_despejo);
    }

    if (doc.containsKey("rotina")) {
        JsonArray rotinaArray = doc["rotina"];
        int i = 0;
        for (JsonObject obj : rotinaArray) {
            if (i >= 4) break;
            
            rotinas[i].hora = obj["hora"];
            rotinas[i].minuto = obj["minuto"];
            rotinas[i].ativo = obj["ativo"];
            sConfig.rotinas[i] = rotinas[i];
            
            Serial.printf("Rotina %d: %02d:%02d - %s\n", 
                i, 
                rotinas[i].hora, 
                rotinas[i].minuto, 
                rotinas[i].ativo ? "Ativo" : "Inativo");
            
            i++;
        }
    }

    server.send(200, "text/plain", "Ok!");
}


void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/salvar", handleJSON);
  server.on("/status", handleStatus);
  server.on("/despejoManual", handleDespejoManual);
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

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

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