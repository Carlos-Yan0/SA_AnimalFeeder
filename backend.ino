#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "niggo.h"
#include <ESP32Servo.h>


// ==============================
// ==== CONFIGURAÇÕES DE PINOS ===
// ==============================
const uint8_t PIN_TRIG    = 20;
const uint8_t PIN_ECHO    = 21;
const uint8_t PIN_BUZZER  = 22;
Servo servoRacao;
const int PIN_SERVO = 23;

// ==============================
// ==== VARIÁVEIS DO SISTEMA ====
// ==============================
// bool statusLED = LOW;
bool eventoExecutado_1623 = false;   // Controle do evento diário

// ==============================
// ==== CREDENCIAIS WI-FI =======
// ==============================
const char* ssid     = "YAN";
const char* password = "123456esp";

WebServer server(80);

// =====================================================
// ===============   MÓDULO: WI-FI   ====================
// =====================================================

void configurarWiFi() {
  WiFi.mode(WIFI_STA); 
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

  WiFi.disconnect(true);
  delay(1000);

  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ===================================================
// ==============   CONFIG: TEMPO   =================
// ===================================================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;   // UTC-3 para Brasil
const int daylightOffset_sec = 0;

// Função modular para pegar hora atual
void pegarHora(int &hora, int &minuto, int &segundo) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    hora = timeinfo.tm_hour;
    minuto = timeinfo.tm_min;
    segundo = timeinfo.tm_sec;
  }
}

// =====================================================
// ==============   MÓDULO: LED/BOTÃO   =================
// =====================================================

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
  servoRacao.write(80);
  delay(3000);
  servoRacao.write(0);

  server.send(200, "text/plain", "despejo Leve Ok");
  Serial.println("Animal Alimentado!");
}

void despejoMedio() {
  Serial.println("Despejando uma quantidade consideravel de ração...");
  servoRacao.write(80);
  delay(5000);
  servoRacao.write(0);

  server.send(200, "text/plain", "despejo medio Ok");
  Serial.println("Animal Alimentado!");
}

void despejoGrande() {
  Serial.println("Despejando Muita ração...");
  servoRacao.write(80);
  chamarAtencao();
  delay(8000);
  servoRacao.write(0);

  server.send(200, "text/plain", "despejo grande Ok");
  Serial.println("Animal Alimentado!");
}

void despejoPersonalizado(int tempo){
  Serial.println("Despejando uma quantidade personalizada de ração...");
  servoRacao.write(80);
  chamarAtencao();
  delay(tempo * 1000);
  servoRacao.write(0);

  server.send(200, "text/plain", "despejo personalizado Ok");
  Serial.println("Animal Alimentado!");
}

void chamarAtencao() {
  // FALTA O SOM DO BUZZER
}

void handleStatus() {
  server.send(200, "text/plain");
}


// =====================================================
// ==============   MÓDULO: PÁGINAS WEB   ===============
// =====================================================

void handleRoot() {
  Serial.println("Acessando página principal");
  server.send(200, "text/html", index_html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/despejoLeve", despejoLeve);
  server.on("/despejoMedio", despejoMedio);
  server.on("/despejoGrande", despejoGrande);
  server.on("/despejoPersonalizado", despejoPersonalizado);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
}

// =====================================================
// ==============   MÓDULO: AUTOMAÇÕES   ===============
// =====================================================

// ===== Variáveis para controle do tempo =====
// unsigned long ledStartTime = 0;
// bool ledTemporizadoAtivo = false;

// void automacaoHoraria() {
//   int h, m, s;
//   pegarHora(h, m, s);

//   // ===== Evento programado para 16:34 =====
//   if (h == 16 && m == 43 && !eventoExecutado_1623) {
//     Serial.println(">>> Evento das 16:34 executado!");

//     // Liga o LED
//     statusLED = HIGH;
//     Serial.println("LED aceso pelo evento programado por 20s");

//     // Inicia contagem
//     ledStartTime = millis();
//     ledTemporizadoAtivo = true;

//     eventoExecutado_1623 = true; // evita repetir no mesmo minuto
//   }

//   // ===== Controle do tempo do LED (20s) =====
//   if (ledTemporizadoAtivo) {
//     if (millis() - ledStartTime >= 20000) {  // 20.000 ms = 20 segundos
//       statusLED = LOW;
//       ledTemporizadoAtivo = false;
//       Serial.println("LED apagado após 20s");
//     }
//   }

//   // ===== Reset diário =====
//   if (h == 0 && m == 0 && s == 0) {
//     eventoExecutado_1623 = false;
//     Serial.println("Eventos diários resetados.");
//   }
// }


void rotinaAutomatica() {
  automacaoHoraria();
}

// =====================================================
// ===================== SETUP =========================
// =====================================================

void setup() {
  Serial.begin(115200);

  servoRacao.attach(PIN_SERVO, 500, 2400);  

  configurarWiFi();

  // Ativa NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Sincronizando horário...");
  delay(2000);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
      Serial.println("Falha ao obter hora via NTP");
  } else {
      Serial.println(&timeinfo, "Hora atual: %H:%M:%S");
  }

  configurarRotas();
  server.begin();

  Serial.println("Servidor Web iniciado!");
}

// =====================================================
// ====================== LOOP =========================
// =====================================================

void loop() {
  server.handleClient();
  rotinaAutomatica();
  delay(10);
}