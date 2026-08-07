#include "index.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// PINOS DO SENSOR ULTRASSÔNICO E PERIFÉRICOS
const int trigPin = 25;
const int echoPin = 26;
const int ledPin = 2;
const int buzzerPin = 27; // Pino do Buzzer (GPIO 27)
const int BOTAO_RESET_WIFI = 14;

// CONFIGURAÇÕES DO POÇO (PADRÃO EM CM)
float profVazia = 300.0; // Distância do sensor até o fundo do poço sem água
float profCheia =
    30.0; // Distância do sensor até a água quando o poço está cheio

// HISTÓRICO LOCAL E PARÂMETROS
int TOTAL_LEITURAS = 10;               // Amostras ultrassônicas por medição (configurável, 1-20)
int MAX_LEITURAS = 30;                 // Tamanho dinâmico do histórico (configurável, 3-50)
const int CAPACIDADE_HISTORICO = 50;   // Capacidade máxima estática do buffer de histórico
float historico[CAPACIDADE_HISTORICO]; // Buffer de histórico
int indiceAtual = 0;                   // Índice atual do buffer de histórico
bool bufferCheio = false;              // Flag para indicar se o buffer está cheio

// CONFIGURAÇÕES DO BANCO DE DADOS TURSO (VALORES PADRÃO SE NÃO SALVO NA NVS)
String turso_url =
    "https://esppoco-olafotte.aws-us-east-1.turso.io/v2/pipeline";
String turso_token =
    "eyJhbGciOiJFZERTQSIsInR5cCI6IkpXVCJ9."
    "eyJhIjoicnciLCJpYXQiOjE3ODMwMzQ0ODksImlkIjoiMDE5ZjI1MjItZjkwMS03NzFmLWEwNm"
    "YtOTZiYjM0MGU5MWZiIiwia2lkIjoid0Y3WjExRUU3UHZwaVdrMTgzeVVsZlgtalFQNTZUMXBa"
    "ZXRiai1vemYtUSIsInJpZCI6ImE2NWEyMzQxLTI1ZmQtNDk1Zi1hMTk3LWE3NDRmNzdhMTJkYi"
    "J9.zk_"
    "wrg7JjcNLgaP3ld0rsSBwSSViROqmBChPsqaspRq1vExHV5TbzqNAFFfyvl64X8nnnYiduIs1M"
    "eN2Qqm0Aw";

// ESTRUTURAS DE SERVIDOR E PREFERENCES
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;
bool tursoOk = false; // Status de integridade da conexão com a nuvem Turso
bool setupConcluidoComSucesso = false; // Flag de trava síncrona do setup

// CONTROLE DE TEMPO NÃO-BLOQUEANTE
unsigned long tempoAnteriorLeitura = 0;
long intervaloLeitura = 10000; // 10 segundos em milissegundos (padrão)

unsigned long tempoAnteriorTurso = 0;
long intervaloTurso =
    300000; // 5 minutos (300 segundos) em milissegundos (padrão)

unsigned long tempoAnteriorReconexao = 0;
const long intervaloReconexao = 30000; // 30 segundos

// ============================================================================
// SINALIZAÇÃO DE DIAGNÓSTICO POR LED (GPIO 2) E BUZZER (GPIO 4)
// ============================================================================

void ativarSinal(int freq = 2000) {
  digitalWrite(ledPin, HIGH);
  tone(buzzerPin, freq);
}

void desativarSinal() {
  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  digitalWrite(buzzerPin, LOW);
}

bool habilitarBeepLeitura =
    false; // Controla a ativação do beep proporcional por leitura

// ============================================================================
// SINALIZAÇÃO SINTETIZADA / EFEITOS SONOROS ESTILO R2-D2
// ============================================================================

// Efeito de Varredura Frequencial / Glissando R2-D2
void varreduraFrequencia(int freqInicial, int freqFinal, int duracaoMs) {
  int passos = 20;
  int delayPasso = duracaoMs / passos;
  if (delayPasso < 1)
    delayPasso = 1;
  int deltaFreq = (freqFinal - freqInicial) / passos;

  for (int i = 0; i <= passos; i++) {
    int freqAtual = freqInicial + (i * deltaFreq);
    ativarSinal(freqAtual);
    delay(delayPasso);
  }
  desativarSinal();
}

// 1. Sucesso Wi-Fi / Boot Conectado: Chirp R2-D2 alegre subindo (1000Hz ->
// 3200Hz)
void sinalizarSucessoWifi() {
  varreduraFrequencia(1000, 3200, 200);
  varreduraFrequencia(1000, 3200, 200);
}

// 2. Erro de Conexão Wi-Fi: Varredura triste decrescente (2400Hz -> 600Hz)
void sinalizarErroWifi() { varreduraFrequencia(2400, 600, 600); }

// 3. Erro no Sensor Ultrassônico: Sirene rápida bitonal (2400Hz <-> 1200Hz)
void sinalizarErroSensor() {
  for (int i = 0; i < 3; i++) {
    ativarSinal(2400);
    delay(60);
    ativarSinal(1200);
    delay(60);
  }
  desativarSinal();
}

// 4. Sucesso no Envio Turso: Bip duplo agudo brilhante (3000Hz)
void sinalizarSucessoTurso() {
  ativarSinal(3000);
  delay(60);
  desativarSinal();
  delay(60);
  ativarSinal(3000);
  delay(60);
  desativarSinal();
}

// 5. Erro no Envio Turso: Vibrato / Oscilação de alerta em rede (1400Hz <->
// 1800Hz)
void sinalizarErroTurso() {
  for (int i = 0; i < 4; i++) {
    ativarSinal(1800);
    delay(50);
    ativarSinal(1400);
    delay(50);
  }
  desativarSinal();
}

// 6. Reset NVS Executado pelo Botão: Varredura de queda livre de sistema
// (3600Hz -> 400Hz)
void sinalizarResetNVS() { varreduraFrequencia(3600, 400, 50); }

// 7. Vinheta sonora de Início do Setup (Power-Up Jingle: C5 - E5 - G5 - C6)
void tocarMusicaInicioSetup() {
  int notas[] = {523, 659, 784, 1047};
  for (int i = 0; i < 4; i++) {
    ativarSinal(notas[i]);
    delay(90);
    desativarSinal();
    delay(30);
  }
}

// 8. Vinheta sonora de Fim do Setup / Sistema Pronto (Victory Fanfare: G5 - C6
// - E6 - G6)
void tocarMusicaFimSetup() {
  int notas[] = {784, 1047, 1319, 1568};
  int duracoes[] = {100, 100, 100, 300};
  for (int i = 0; i < 4; i++) {
    ativarSinal(notas[i]);
    delay(duracoes[i]);
    desativarSinal();
    delay(40);
  }
}

// 10. Transmitir dígito numérico via áudio (1-9 = N beeps agudos, 0 = 1 toque
// grave curto)
void emitirBeepDigito(int d) {
  if (d == 0) {
    // Digito zero: toque grave e curto (500 Hz por 60ms)
    ativarSinal(500);
    delay(60);
    desativarSinal();
    delay(200);
  } else {
    // Dígitos 1 a 9: D beeps agudos (2200 Hz por 120ms com pausa de 120ms)
    for (int i = 0; i < d; i++) {
      ativarSinal(2200);
      delay(240);
      desativarSinal();
      delay(240);
    }
  }
}

// Transmitir um octeto de IP (Centena -> Dezena -> Unidade)
void emitirAudioOcteto(int val) {
  int c = val / 100;
  int d = (val % 100) / 10;
  int u = val % 10;

  // Centenas (se houver)
  if (c > 0) {
    emitirBeepDigito(c);
    delay(1000); // Pausa entre centena e dezena
  }

  // Dezenas (se houver centena ou dezena)
  if (c > 0 || d > 0) {
    emitirBeepDigito(d);
    delay(1000); // Pausa entre dezena e unidade
  }

  // Unidades
  emitirBeepDigito(u);
}

// Sinal sonoro característico do Ponto (".") entre octetos de IP
void emitirSinalPontoIP() {
  delay(300);
  // Duplo bip agudo em varredura rápida para indicar o PONTO "."
  ativarSinal(3200);
  delay(70);
  desativarSinal();
  delay(70);
  ativarSinal(3600);
  delay(70);
  desativarSinal();
  delay(600); // Pausa após o ponto antes do próximo octeto
}

// Anuncia os 2 últimos grupos do IP via áudio no Buzzer
void anunciarIpBuzzer() {
  if (WiFi.status() != WL_CONNECTED)
    return;
  IPAddress ip = WiFi.localIP();
  int oct3 = ip[2]; // 3º grupo do IP (ex: 1 ou 0 em 192.168.1.105)
  int oct4 = ip[3]; // 4º grupo do IP (ex: 105 em 192.168.1.105)

  Serial.printf("\n=============================================\n");
  Serial.printf(" [IP NETWORK] IP Local Completo: %s\n", ip.toString().c_str());
  Serial.printf(" [IP AUDIO] Anunciando 2 últimos grupos: %d . %d\n", oct3,
                oct4);
  Serial.printf("=============================================\n\n");

  delay(1000); // Pausa inicial antes de começar a transmitir o IP
  emitirAudioOcteto(oct3);
  emitirSinalPontoIP();
  emitirAudioOcteto(oct4);
}

// 9. Beep com frequência proporcional ao nível da água (Tom agudo = poço cheio,
// Tom grave = poço vazio)
void sinalizarBeepNivelProporcional(float dist) {
  if (!habilitarBeepLeitura || dist < 0)
    return;
  float distClamped = constrain(dist, profCheia, profVazia);

  // Mapeamento proporcional: profCheia (30cm / poço cheio) -> 2800 Hz (agudo)
  //                          profVazia (300cm / poço vazio) -> 300 Hz (grave)
  long freq = map((long)(distClamped * 10.0), (long)(profCheia * 10.0),
                  (long)(profVazia * 10.0), 2800, 300);

  ativarSinal((int)freq);
  delay(150);
  desativarSinal();
}

// Calcula porcentagem do nível da água (0% a 100%)
float calcularPorcentagem(float dist) {
  if (dist < 0)
    return -1.0;
  if (profVazia <= profCheia)
    return 0.0;

  float pct = ((profVazia - dist) / (profVazia - profCheia)) * 100.0;
  if (pct < 0.0)
    pct = 0.0;
  if (pct > 100.0)
    pct = 100.0;
  return pct;
}

// Realiza leituras locais e escolhe a maior distância válida (evita ecos de
// ressonância)
float lerDistanciaSemRessonancia() {
  float maiorDistanciaEncontrada = -1.0;
  int leiturasValidas = 0;

  digitalWrite(ledPin, HIGH);

  for (int i = 0; i < TOTAL_LEITURAS; i++) {
    server.handleClient(); // Processa requisições web durante a amostragem

    digitalWrite(trigPin, LOW);
    delayMicroseconds(5);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duracao = pulseIn(echoPin, HIGH, 25000);
    float distancia = duracao * 0.0343 / 2.0;

    if (distancia >= 15.0 && distancia <= 500.0) {
      leiturasValidas++;
      if (distancia > maiorDistanciaEncontrada) {
        maiorDistanciaEncontrada = distancia;
      }
    }
    delay(100);
  }

  digitalWrite(ledPin, LOW);

  if (leiturasValidas > 0) {
    return maiorDistanciaEncontrada;
  }
  return -1.0;
}

void adicionarLeitura(float valor) {
  if (valor != -1.0) {
    historico[indiceAtual] = valor;
    indiceAtual = (indiceAtual + 1) % MAX_LEITURAS;
    if (indiceAtual == 0) {
      bufferCheio = true;
    }
  }
}

// Retorna o 3º maior valor das últimas leituras para filtragem de oscilações
float obterTerceiroMaiorDistanciaUltimas30() {
  int totalDisponivel = bufferCheio ? MAX_LEITURAS : indiceAtual;
  if (totalDisponivel < 3)
    return -1.0;

  int limiteAnalise = (totalDisponivel < MAX_LEITURAS) ? totalDisponivel : MAX_LEITURAS;
  float amostras[50];
  int qtdAmostras = 0;

  for (int i = 0; i < limiteAnalise; i++) {
    int idx = (indiceAtual - 1 - i + MAX_LEITURAS) % MAX_LEITURAS;
    if (historico[idx] != -1.0) {
      amostras[qtdAmostras] = historico[idx];
      qtdAmostras++;
    }
  }

  if (qtdAmostras < 3)
    return -1.0;

  // Ordenação Bubble Sort (Crescente)
  for (int i = 0; i < qtdAmostras - 1; i++) {
    for (int j = i + 1; j < qtdAmostras; j++) {
      if (amostras[i] > amostras[j]) {
        float temp = amostras[i];
        amostras[i] = amostras[j];
        amostras[j] = temp;
      }
    }
  }

  return amostras[qtdAmostras - 3]; // 3º Maior
}

// Resposta JSON limpa e estruturada via ArduinoJson v7
String getHistoricoJSON() {
  JsonDocument doc;
  float ultimaDist =
      (indiceAtual > 0 || bufferCheio)
          ? historico[(indiceAtual - 1 + MAX_LEITURAS) % MAX_LEITURAS]
          : -1.0;

  doc["distancia_cm"] = ultimaDist;
  doc["nivel_pct"] = calcularPorcentagem(ultimaDist);
  doc["beep_ativo"] = habilitarBeepLeitura;
  doc["total_leituras"] = TOTAL_LEITURAS;
  doc["max_leituras"] = MAX_LEITURAS;
  doc["intervalo_leitura_s"] = intervaloLeitura / 1000;
  doc["intervalo_turso_m"] = intervaloTurso / 60000;

  JsonArray arr = doc["historico_cm"].to<JsonArray>();
  int total = bufferCheio ? MAX_LEITURAS : indiceAtual;
  for (int i = 0; i < total; i++) {
    int idx = bufferCheio ? (indiceAtual + i) % MAX_LEITURAS : i;
    arr.add(historico[idx]);
  }

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

// Envio HTTPS para o Turso (ArduinoJson v7)
void enviarDadosTurso(float nivel) {
  if (nivel == -1.0 || WiFi.status() != WL_CONNECTED)
    return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (http.begin(client, turso_url)) {
    http.addHeader("Content-Type", "application/json");
    String authHeader = "Bearer " + turso_token;
    http.addHeader("Authorization", authHeader);

    String querySql =
        "INSERT INTO leituras_poco (nivel_cm, status_bomba) VALUES (";
    querySql += String(nivel, 2);
    querySql += ", 'MONITORANDO');";

    JsonDocument doc;
    JsonArray requests = doc["requests"].to<JsonArray>();

    JsonObject req1 = requests.add<JsonObject>();
    req1["type"] = "execute";
    JsonObject stmt = req1["stmt"].to<JsonObject>();
    stmt["sql"] = querySql;

    JsonObject req2 = requests.add<JsonObject>();
    req2["type"] = "close";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);
    if (httpCode == 200) {
      Serial.println("[Turso] SUCESSO: Leitura salva na nuvem com exito.");
      sinalizarSucessoTurso();
      tursoOk = true;
    } else {
      Serial.printf("[Turso] ERRO HTTP (%d)\n", httpCode);
      sinalizarErroTurso();
      tursoOk = false;
    }
    http.end();
  } else {
    tursoOk = false;
  }
}

// Teste inicial de conexão Turso
bool testarConexaoTurso() {
  if (WiFi.status() != WL_CONNECTED) {
    tursoOk = false;
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (http.begin(client, turso_url)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + turso_token);

    JsonDocument doc;
    JsonArray requests = doc["requests"].to<JsonArray>();

    JsonObject req1 = requests.add<JsonObject>();
    req1["type"] = "execute";
    JsonObject stmt = req1["stmt"].to<JsonObject>();
    stmt["sql"] = "SELECT 1;";

    JsonObject req2 = requests.add<JsonObject>();
    req2["type"] = "close";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);
    if (httpCode == 200) {
      Serial.println("[Turso Teste] SUCESSO: Conexão e token validados.");
      sinalizarSucessoTurso();
      tursoOk = true;
    } else {
      Serial.printf("[Turso Teste] ALERTA: Resposta HTTP %d\n", httpCode);
      sinalizarErroTurso();
      tursoOk = false;
    }
    http.end();
  } else {
    tursoOk = false;
  }
  return tursoOk;
}

// Portal Captivo DNS e Servidor de Configuração
void iniciarPortalConfiguracao() {
  Serial.println("\n--- Entrando em modo de Configuração (Access Point + "
                 "Captive Portal) ---");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Config_Sensor_Poco");
  IPAddress apIP = WiFi.softAPIP();

  // Inicia Servidor DNS Captivo na porta 53 para redirecionamento automático
  dnsServer.start(53, "*", apIP);

  auto handlePortal = [apIP]() {
    String html =
        "<!DOCTYPE html><html><head><meta charset='utf-8'><meta "
        "name='viewport' content='width=device-width, initial-scale=1'>";
    html +=
        "<style>body{font-family:sans-serif;background:#0f172a;color:#f8fafc;"
        "padding:20px;} "
        ".card{background:#1e293b;padding:20px;border-radius:10px;max-width:"
        "400px;margin:0 auto;} input{width:100%;padding:10px;margin:8px "
        "0;border-radius:5px;border:1px solid "
        "#334155;background:#0f172a;color:#fff;} "
        "input[type=submit]{background:#38bdf8;color:#000;font-weight:bold;"
        "cursor:pointer;margin-top:15px;}</style></head><body>";
    html += "<div class='card'><h2>⚙️ Configuração do Sensor</h2>";
    html += "<form action='/salvar' method='POST'>";
    html += "<label>Rede Wi-Fi (SSID):</label><input type='text' name='ssid' "
            "required>";
    html += "<label>Senha do Wi-Fi:</label><input type='password' name='pass' "
            "required>";
    html += "<hr style='border-color:#334155;margin:15px 0;'>";
    html += "<label>Profundidade Vazia (cm):</label><input type='number' "
            "name='vazia' value='" +
            String(profVazia, 0) + "'>";
    long intLeituraSeg = intervaloLeitura / 1000;
    long intTursoMin = intervaloTurso / 60000;

    html += "<label>Profundidade Cheia (cm):</label><input type='number' "
            "name='cheia' value='" +
            String(profCheia, 0) + "'>";
    html += "<label>Amostras Ultrassônicas (TOTAL_LEITURAS):</label><input type='number' "
            "name='tot_leituras' value='" +
            String(TOTAL_LEITURAS) + "' min='1' max='20'>";
    html += "<label>Tamanho do Histórico (MAX_LEITURAS):</label><input type='number' "
            "name='max_leituras' value='" +
            String(MAX_LEITURAS) + "' min='3' max='50'>";
    html += "<label>Intervalo Leitura Local (segundos):</label><input "
            "type='number' "
            "name='int_leitura' value='" +
            String(intLeituraSeg) + "' min='3'>";
    html +=
        "<label>Intervalo Envio Turso (minutos):</label><input type='number' "
        "name='int_turso' value='" +
        String(intTursoMin) + "' min='1'>";
    html +=
        "<label style='display:flex;align-items:center;gap:10px;margin:12px "
        "0;cursor:pointer;'>"
        "<input type='checkbox' name='beep' value='1' "
        "style='width:auto;margin:0;' " +
        String(habilitarBeepLeitura ? "checked" : "") +
        ">"
        "<span>Ativar Beep Sonoro nas Leituras</span></label>";
    html += "<label>Token Turso (Opcional):</label><input type='text' "
            "name='token' placeholder='Manter atual'>";
    html += "<input type='submit' value='Salvar e Conectar'>";
    html += "</form></div></body></html>";
    server.send(200, "text/html", html);
  };

  server.on("/", HTTP_GET, handlePortal);
  server.on("/generate_204", HTTP_GET,
            handlePortal); // Suporte para captive portal Android
  server.on("/redirect", HTTP_GET, handlePortal);

  server.on("/salvar", HTTP_POST, []() {
    String reqSsid = server.arg("ssid");
    String reqPass = server.arg("pass");
    String reqVazia = server.arg("vazia");
    String reqCheia = server.arg("cheia");
    String reqToken = server.arg("token");
    String reqTotLeituras = server.arg("tot_leituras");
    String reqMaxLeituras = server.arg("max_leituras");
    String reqIntLeitura = server.arg("int_leitura");
    String reqIntTurso = server.arg("int_turso");
    bool reqBeep = server.hasArg("beep");

    preferences.begin("system-config", false);
    if (reqSsid.length() > 0)
      preferences.putString("ssid", reqSsid);
    if (reqPass.length() > 0)
      preferences.putString("pass", reqPass);
    if (reqVazia.length() > 0)
      preferences.putFloat("vazia", reqVazia.toFloat());
    if (reqCheia.length() > 0)
      preferences.putFloat("cheia", reqCheia.toFloat());
    if (reqToken.length() > 0)
      preferences.putString("token", reqToken);
    if (reqTotLeituras.length() > 0 && reqTotLeituras.toInt() > 0)
      preferences.putInt("tot_leituras", reqTotLeituras.toInt());
    if (reqMaxLeituras.length() > 0 && reqMaxLeituras.toInt() > 0)
      preferences.putInt("max_leituras", reqMaxLeituras.toInt());
    if (reqIntLeitura.length() > 0 && reqIntLeitura.toInt() >= 3)
      preferences.putLong("int_leitura", reqIntLeitura.toInt() * 1000L);
    else if (reqIntLeitura.length() > 0)
      preferences.putLong("int_leitura", 3000L); // Piso de segurança mínimo 3s
    if (reqIntTurso.length() > 0 && reqIntTurso.toInt() > 0)
      preferences.putLong("int_turso", reqIntTurso.toInt() * 60000L);
    preferences.putBool("beep", reqBeep);
    preferences.end();

    String html = "<html><body "
                  "style='font-family:sans-serif;background:#0f172a;color:#"
                  "f8fafc;text-align:center;padding:40px;'>";
    html += "<h2 style='color:#22c55e;'>Dados Salvos com Sucesso!</h2>";
    html += "<p>O ESP32 está reiniciando para conectar em <b>" + reqSsid +
            "</b>.</p>";
    html += "<p>Acesse em seguida: "
            "<b>http://monitorpoco.local</b></p></body></html>";

    server.send(200, "text/html", html);
    delay(1500);
    ESP.restart();
  });

  server.onNotFound(handlePortal);
  server.begin();

  unsigned long tempoBlink = 0;
  bool estadoLed = LOW;

  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();

    if (millis() - tempoBlink >= 1000) {
      tempoBlink = millis();
      estadoLed = !estadoLed;
      digitalWrite(ledPin, estadoLed);
    }
    delay(1);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  noTone(buzzerPin);
  pinMode(BOTAO_RESET_WIFI, INPUT_PULLUP);

  // Toca vinheta de inicialização do hardware
  tocarMusicaInicioSetup();

  // 1. CHECAGEM DO BOTÃO DE RESET (Pressionado ao ligar)
  if (digitalRead(BOTAO_RESET_WIFI) == LOW) {
    delay(100);
    if (digitalRead(BOTAO_RESET_WIFI) == LOW) {
      Serial.println("Botão de Reset Pressionado! Limpando configurações...");
      sinalizarResetNVS();
      preferences.begin("system-config", false);
      preferences.clear();
      preferences.end();
      iniciarPortalConfiguracao();
      return;
    }
  }

  // 2. LEITURA DE CONFIGURAÇÕES SALVAS
  preferences.begin("system-config", true);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  profVazia = preferences.getFloat("vazia", 300.0);
  profCheia = preferences.getFloat("cheia", 30.0);
  TOTAL_LEITURAS = preferences.getInt("tot_leituras", 10);
  MAX_LEITURAS = preferences.getInt("max_leituras", 30);
  if (TOTAL_LEITURAS < 1) TOTAL_LEITURAS = 1;
  if (TOTAL_LEITURAS > 20) TOTAL_LEITURAS = 20;
  if (MAX_LEITURAS < 3) MAX_LEITURAS = 3;
  if (MAX_LEITURAS > 50) MAX_LEITURAS = 50;

  habilitarBeepLeitura = preferences.getBool("beep", false);
  intervaloLeitura = preferences.getLong("int_leitura", 10000L);
  if (intervaloLeitura < 3000L) {
    intervaloLeitura = 3000L; // Piso de segurança: Mínimo 3 segundos para não travar o servidor web
  }
  intervaloTurso = preferences.getLong("int_turso", 300000L);
  String savedToken = preferences.getString("token", "");
  if (savedToken.length() > 0) {
    turso_token = savedToken;
  }
  preferences.end();

  // 3. TENTATIVA DE CONEXÃO WI-FI
  if (ssid != "") {
    Serial.print("Conectando ao Wi-Fi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
      delay(500);
      Serial.print(".");
      tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi Conectado com Sucesso!");
      sinalizarSucessoWifi();

      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      time_t now = time(nullptr);
      int timeoutNTP = 0;
      while (now < 8 * 3600 * 2 && timeoutNTP < 15) {
        delay(500);
        now = time(nullptr);
        timeoutNTP++;
      }

      if (MDNS.begin("monitorpoco")) {
        Serial.println("mDNS Ativo: http://monitorpoco.local");
      }

      server.on("/", HTTP_GET,
                []() { server.send_P(200, "text/html", index_html); });
      server.on("/dados", HTTP_GET, []() {
        server.send(200, "application/json", getHistoricoJSON());
      });
      server.on("/config_beep", HTTP_POST, []() {
        if (server.hasArg("ativo")) {
          habilitarBeepLeitura =
              (server.arg("ativo") == "1" || server.arg("ativo") == "true");
          preferences.begin("system-config", false);
          preferences.putBool("beep", habilitarBeepLeitura);
          preferences.end();
          Serial.printf(
              "[Web Server] Beep de leitura alterado via Dashboard Web: %s\n",
              habilitarBeepLeitura ? "LIGADO" : "DESLIGADO");
          server.send(200, "application/json", "{\"sucesso\":true}");
        } else {
          server.send(400, "application/json",
                      "{\"erro\":\"Parametro ausente\"}");
        }
      });
      server.on("/config_parametros", HTTP_POST, []() {
        preferences.begin("system-config", false);
        if (server.hasArg("tot_leituras")) {
          int v = server.arg("tot_leituras").toInt();
          if (v >= 1 && v <= 20) { TOTAL_LEITURAS = v; preferences.putInt("tot_leituras", TOTAL_LEITURAS); }
        }
        if (server.hasArg("max_leituras")) {
          int v = server.arg("max_leituras").toInt();
          if (v >= 3 && v <= 50) {
            MAX_LEITURAS = v;
            preferences.putInt("max_leituras", MAX_LEITURAS);
            indiceAtual = 0;
            bufferCheio = false;
          }
        }
        if (server.hasArg("int_leitura")) {
          long v = server.arg("int_leitura").toInt();
          if (v < 3) v = 3; // Piso mínimo 3s
          intervaloLeitura = v * 1000L;
          preferences.putLong("int_leitura", intervaloLeitura);
        }
        if (server.hasArg("int_turso")) {
          long v = server.arg("int_turso").toInt();
          if (v >= 1) { intervaloTurso = v * 60000L; preferences.putLong("int_turso", intervaloTurso); }
        }
        preferences.end();
        Serial.println("[Web Server] Parâmetros de amostragem e tempo atualizados via Web!");
        server.send(200, "application/json", "{\"sucesso\":true}");
      });
      server.begin();

      if (testarConexaoTurso()) {
        setupConcluidoComSucesso = true;
        delay(500);
        tocarMusicaFimSetup();
        anunciarIpBuzzer();
        Serial.println("[System] Setup concluído com SUCESSO! Transicionando "
                       "para o loop.");
      } else {
        Serial.println("[System] Wi-Fi OK, mas falha no teste Turso. "
                       "Aguardando validação para concluir setup...");
      }
      return;
    } else {
      Serial.println("\nFalha inicial de conexão Wi-Fi. Aguardando conexão "
                     "para concluir o setup...");
      sinalizarErroWifi();
    }
  }

  // Se não houver Wi-Fi configurado
  if (ssid == "") {
    iniciarPortalConfiguracao();
  }
}

void loop() {
  server.handleClient();
  unsigned long tempoAtual = millis();

  // ============================================================================
  // TRAVA SÍNCRONA DE SETUP: Bloqueia o loop até Wi-Fi + Turso estarem OK
  // ============================================================================
  if (!setupConcluidoComSucesso) {
    if (WiFi.status() != WL_CONNECTED &&
        (tempoAtual - tempoAnteriorReconexao >= intervaloReconexao)) {
      tempoAnteriorReconexao = tempoAtual;
      Serial.println(
          "[Setup Incompleto] Tentando conectar/reconectar ao Wi-Fi...");
      sinalizarErroWifi();
      WiFi.reconnect();
    } else if (WiFi.status() == WL_CONNECTED && !tursoOk &&
               (tempoAtual - tempoAnteriorReconexao >= 10000)) {
      tempoAnteriorReconexao = tempoAtual;
      Serial.println("[Setup Incompleto] Wi-Fi OK. Validando conexão com a "
                     "nuvem Turso...");
      if (testarConexaoTurso()) {
        setupConcluidoComSucesso = true;
        tocarMusicaFimSetup();
        anunciarIpBuzzer();
        Serial.println("[System] Setup concluído com SUCESSO! Liberando loop "
                       "de monitoramento.");
      }
    }
    return; // TRAVA O LOOP: Nenhuma leitura ou envio ocorre até o Setup
            // concluir com sucesso!
  }

  // ============================================================================
  // LOOP PRINCIPAL DE MONITORAMENTO (Executado apenas após Setup OK)
  // ============================================================================

  // Reconexão Wi-Fi Assíncrona se cair durante a execução normal
  if (WiFi.status() != WL_CONNECTED &&
      (tempoAtual - tempoAnteriorReconexao >= intervaloReconexao)) {
    tempoAnteriorReconexao = tempoAtual;
    Serial.println(
        "[Wi-Fi] Tentando reconectar à rede salva em segundo plano...");
    WiFi.reconnect();
  }

  // Temporizador 1: Leitura Local do Sensor (A cada 10 segundos)
  if (tempoAtual - tempoAnteriorLeitura >= intervaloLeitura) {
    tempoAnteriorLeitura = tempoAtual;

    // Checagem rigorosa de conexões: Aborta leituras se Wi-Fi ou Turso
    // estiverem com problema
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[Leitura Abortada] Sem conexão Wi-Fi ativa.");
      sinalizarErroWifi();
    } else if (!tursoOk) {
      Serial.println("[Leitura Abortada] Conexão com Turso indisponível. "
                     "Tentando re-validar...");
      sinalizarErroTurso();
      testarConexaoTurso(); // Tenta re-validar o Turso para liberar leituras na
                            // próxima rodada
    } else {
      float novaLeitura = lerDistanciaSemRessonancia();

      if (novaLeitura == -1.0) {
        Serial.println("Erro na leitura ultrassônica.");
        sinalizarErroSensor();
      } else {
        adicionarLeitura(novaLeitura);
        float pct = calcularPorcentagem(novaLeitura);
        Serial.printf("Leitura Local: %.1f cm (Nível: %.1f%%)\n", novaLeitura,
                      pct);
        sinalizarBeepNivelProporcional(novaLeitura);
      }
    }
  }

  // Temporizador 2: Envio para Nuvem Turso (A cada 5 minutos)
  if (tempoAtual - tempoAnteriorTurso >= intervaloTurso) {
    tempoAnteriorTurso = tempoAtual;
    float valorFiltrado = obterTerceiroMaiorDistanciaUltimas30();

    if (valorFiltrado != -1.0) {
      Serial.printf("[Turso] Transmitindo leitura filtrada (5 min): %.1f cm\n",
                    valorFiltrado);
      enviarDadosTurso(valorFiltrado);
    }
  }
}