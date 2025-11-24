#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "niggo.h"

// ==============================
// ==== CONFIGURAÇÕES DE PINOS ===
// ==============================
const uint8_t PIN_LED     = 18;
const uint8_t PIN_BUTTON  = 19;
const uint8_t PIN_TRIG    = 20;
const uint8_t PIN_ECHO    = 21;
const uint8_t PIN_BUZZER  = 22;

// ==============================
// ==== VARIÁVEIS DO SISTEMA ====
// ==============================
bool statusLED = LOW;
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

void atualizarLED() {
  digitalWrite(PIN_LED, statusLED);
}

void LEDon() {
  statusLED = HIGH;
  atualizarLED();
  server.send(200, "text/plain", "on");
  Serial.println("LED ligado via web");
}

void LEDoff() {
  statusLED = LOW;
  atualizarLED();
  server.send(200, "text/plain", "off");
  Serial.println("LED desligado via web");
}

void handleStatus() {
  server.send(200, "text/plain", statusLED ? "on" : "off");
}

void verificarBotao() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    while (digitalRead(PIN_BUTTON) == LOW); // Espera soltar

    statusLED = !statusLED;
    atualizarLED();

    Serial.println("Botão pressionado — LED alternado");
  }
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
  server.on("/on", LEDon);
  server.on("/off", LEDoff);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
}

// =====================================================
// ==============   MÓDULO: AUTOMAÇÕES   ===============
// =====================================================

// ===== Variáveis para controle do tempo =====
unsigned long ledStartTime = 0;
bool ledTemporizadoAtivo = false;

void automacaoHoraria() {
  int h, m, s;
  pegarHora(h, m, s);

  // ===== Evento programado para 16:34 =====
  if (h == 16 && m == 43 && !eventoExecutado_1623) {
    Serial.println(">>> Evento das 16:34 executado!");

    // Liga o LED
    statusLED = HIGH;
    atualizarLED();
    Serial.println("LED aceso pelo evento programado por 20s");

    // Inicia contagem
    ledStartTime = millis();
    ledTemporizadoAtivo = true;

    eventoExecutado_1623 = true; // evita repetir no mesmo minuto
  }

  // ===== Controle do tempo do LED (20s) =====
  if (ledTemporizadoAtivo) {
    if (millis() - ledStartTime >= 20000) {  // 20.000 ms = 20 segundos
      statusLED = LOW;
      atualizarLED();
      ledTemporizadoAtivo = false;
      Serial.println("LED apagado após 20s");
    }
  }

  // ===== Reset diário =====
  if (h == 0 && m == 0 && s == 0) {
    eventoExecutado_1623 = false;
    Serial.println("Eventos diários resetados.");
  }
}


void rotinaAutomatica() {
  automacaoHoraria();
}

// =====================================================
// ===================== SETUP =========================
// =====================================================

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);

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
  verificarBotao();
  rotinaAutomatica();
  delay(10);
}