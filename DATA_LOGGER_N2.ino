/*
 * ============================================================
 *  SUPERVISAO IoT - DEVICE ESP32  (Projeto N2)
 *  Le DHT + LDR, publica via MQTT no IoT Agent (FIWARE) e
 *  executa o comando de alerta recebido (LED azul + buzzer).
 *  Wi-Fi e IP do broker sao configurados NO DEVICE (LCD + botoes)
 *  e ficam salvos na flash (Preferences).
 * ============================================================
 *  PINAGEM
 *    GPIO4   DHT (dados)
 *    GPIO34  LDR (AO)  -> ADC1, funciona com Wi-Fi ligado
 *    GPIO26  LED azul (resistor 220R)
 *    GPIO25  Buzzer
 *    GPIO21  SDA (LCD I2C)     GPIO22  SCL (LCD I2C)
 *    GPIO32  Botao CIMA   GPIO33  Botao BAIXO   GPIO27  Botao OK
 *            (INPUT_PULLUP, outro terminal no GND)
 *
 *  TOPICOS (padrao FIWARE Made Simple, UltraLight 2.0)
 *    publica   /TEF/<DEVICE_ID>/attrs     payload: t|25.3|h|60|l|45
 *    recebe    /TEF/<DEVICE_ID>/cmd       payload: <DEVICE_ID>@alerta|N
 *    responde  /TEF/<DEVICE_ID>/cmdexe    payload: <DEVICE_ID>@alerta|ok
 *
 *  Comando "alerta" (N): 0 = normal, 1 = temperatura,
 *                        2 = umidade, 3 = luminosidade
 *  (a prioridade entre anomalias e decidida no front-end)
 *
 *  USO DOS BOTOES: na tela principal, OK abre o menu de configuracao.
 *  CIMA/BAIXO navega, OK confirma.
 * ============================================================
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ------------------ FIXOS ------------------
const int  BROKER_PORT = 1883;
const char* DEVICE_ID  = "datalogger001";        // igual ao provisionado no IoT Agent

// ------------------ PINOS ------------------
#define DHT_PIN     4
#define LDR_PIN     34
#define LED_PIN     26
#define BUZZER_PIN  25
#define BTN_UP      32
#define BTN_DOWN    33
#define BTN_OK      27

// Wokwi so tem DHT22 e so conecta em "Wokwi-GUEST" (canal 6).
// No hardware real (DHT11): use 0
#define SIMULACAO_WOKWI 0
#if SIMULACAO_WOKWI
  #define DHT_TYPE DHT22
  #define WIFI_CANAL 6
#else
  #define DHT_TYPE DHT11
  #define WIFI_CANAL 0
#endif

#define LDR_INVERTIDO 0   // 1 se mais luz => leitura menor (confira no simulador)

// ------------------ ALERTAS: LED pisca + tom do buzzer ------------------
//                    N:  0     1(temp) 2(umid) 3(luz)
const int  TOM_HZ[4]   = {0,    1000,   1500,   2000};
const int  PISCA_MS[4] = {0,     200,    500,   1000};
const char* NOME_ALERTA[4] = {"NORMAL", "TEMP", "UMID", "LUZ"};

// ------------------ OBJETOS / ESTADO ------------------
WiFiClient espClient;
PubSubClient mqtt(espClient);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Preferences prefs;

// Configuracao salva na flash
char cfgSsid[33]   = "";
char cfgPass[65]   = "";
char cfgBroker[40] = "0.0.0.0";           // IP publico da EC2 (muda a cada sessao)

String topicAttrs, topicCmd, topicCmdExe;
uint8_t alertaAtual = 0;
unsigned long tPublica = 0, tPisca = 0, tTela = 0, tMqtt = 0;
bool ledLigado = false;
const unsigned long INTERVALO_PUBLICA = 5000;

float ultT = NAN, ultH = NAN;
int   ultL = 0;

// Caracteres disponiveis ao digitar (SSID, senha, IP)
const char* CHARSET_TEXTO = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%&*-_.+= ";
const char* CHARSET_IP    = "0123456789.";

#define B_UP   0
#define B_DOWN 1
#define B_OK   2

// ============================================================
//                       LCD E BOTOES
// ============================================================

// Escreve uma linha de 16 colunas so se o texto mudou (evita flicker)
char cacheLcd[2][17];
void lcdLinha(uint8_t row, const char* txt) {
  char buf[17];
  uint8_t i = 0;
  for (; i < 16 && txt[i]; i++) buf[i] = txt[i];
  for (; i < 16; i++) buf[i] = ' ';
  buf[16] = 0;
  if (memcmp(buf, cacheLcd[row], 17) != 0) {
    memcpy(cacheLcd[row], buf, 17);
    lcd.setCursor(0, row);
    lcd.print(buf);
  }
}

// Le botoes com debounce; retorna B_UP/B_DOWN/B_OK ou -1 (borda de descida)
int8_t lerBotao() {
  static const uint8_t pinos[3] = {BTN_UP, BTN_DOWN, BTN_OK};
  static bool ant[3] = {HIGH, HIGH, HIGH};
  static unsigned long t[3] = {0, 0, 0};
  for (uint8_t i = 0; i < 3; i++) {
    bool s = digitalRead(pinos[i]);
    if (s != ant[i] && millis() - t[i] > 40) {
      t[i] = millis();
      ant[i] = s;
      if (s == LOW) return i;
    }
  }
  return -1;
}

// Espera um botao (mantem o MQTT vivo enquanto o menu esta aberto)
int8_t esperaBotao() {
  int8_t b;
  while ((b = lerBotao()) < 0) { mqtt.loop(); delay(5); }
  return b;
}

// ============================================================
//                    CONFIGURACAO (FLASH)
// ============================================================
void carregarConfig() {
  prefs.begin("iot", false);
  prefs.getString("ssid", cfgSsid, sizeof cfgSsid);
  prefs.getString("pass", cfgPass, sizeof cfgPass);
  String b = prefs.getString("broker", cfgBroker);
  b.toCharArray(cfgBroker, sizeof cfgBroker);
}

void salvarConfig() {
  prefs.putString("ssid", cfgSsid);
  prefs.putString("pass", cfgPass);
  prefs.putString("broker", cfgBroker);
}

// Editor de texto: CIMA/BAIXO escolhe o caractere, OK adiciona.
// Itens especiais no fim da lista: <DEL> apaga o ultimo, <FIM> termina.
void editarTexto(const char* titulo, char* buf, size_t maxLen, const char* charset) {
  size_t n = strlen(buf);
  int tam = strlen(charset);
  int idx = 0;
  while (true) {
    char cand[8];
    if (idx < tam)       snprintf(cand, sizeof cand, "%c", charset[idx]);
    else if (idx == tam) strcpy(cand, "<DEL>");
    else                 strcpy(cand, "<FIM>");
    char l0[24];
    snprintf(l0, sizeof l0, "%s %s", titulo, cand);
    lcdLinha(0, l0);
    lcdLinha(1, n > 16 ? buf + (n - 16) : buf);     // mostra o fim do texto

    int8_t b = esperaBotao();
    if (b == B_UP)        idx = (idx + 1) % (tam + 2);
    else if (b == B_DOWN) idx = (idx + tam + 1) % (tam + 2);
    else {
      if (idx < tam)       { if (n < maxLen - 1) { buf[n++] = charset[idx]; buf[n] = 0; } }
      else if (idx == tam) { if (n > 0) buf[--n] = 0; }
      else return;
    }
  }
}

// Escaneia redes e deixa escolher uma da lista (ou digitar o SSID)
void escolherRede() {
  lcdLinha(0, "Buscando redes");
  lcdLinha(1, "");
  int n = WiFi.scanNetworks();
  if (n < 0) n = 0;
  int total = n + 2;               // + "<digitar SSID>" + "<cancelar>"
  int sel = 0;
  while (true) {
    char l0[20];
    snprintf(l0, sizeof l0, "Redes %d/%d", sel + 1, total);
    lcdLinha(0, l0);
    if (sel < n)           lcdLinha(1, WiFi.SSID(sel).c_str());
    else if (sel == n)     lcdLinha(1, "<digitar SSID>");
    else                   lcdLinha(1, "<cancelar>");

    int8_t b = esperaBotao();
    if (b == B_UP)        sel = (sel + 1) % total;
    else if (b == B_DOWN) sel = (sel + total - 1) % total;
    else {
      if (sel < n)         { WiFi.SSID(sel).toCharArray(cfgSsid, sizeof cfgSsid); }
      else if (sel == n)   { editarTexto("SSID", cfgSsid, sizeof cfgSsid, CHARSET_TEXTO); }
      salvarConfig();
      return;
    }
  }
}

// Menu principal de configuracao
void menuConfig() {
  const char* ITENS[4] = {"Rede Wi-Fi", "Senha", "IP do broker", "Conectar e sair"};
  int item = 0;
  while (true) {
    char l0[20];
    snprintf(l0, sizeof l0, ">%s", ITENS[item]);
    lcdLinha(0, l0);
    char l1[40];
    if (item == 0)      snprintf(l1, sizeof l1, "%s", cfgSsid[0] ? cfgSsid : "(vazio)");
    else if (item == 1) { size_t k = strlen(cfgPass); for (size_t i = 0; i < k && i < 16; i++) l1[i] = '*'; l1[k < 16 ? k : 16] = 0; }
    else if (item == 2) snprintf(l1, sizeof l1, "%s", cfgBroker);
    else                snprintf(l1, sizeof l1, "OK = conectar");
    lcdLinha(1, l1);

    int8_t b = esperaBotao();
    if (b == B_UP)        item = (item + 3) % 4;
    else if (b == B_DOWN) item = (item + 1) % 4;
    else {
      if (item == 0)      escolherRede();
      else if (item == 1) { editarTexto("Senha", cfgPass, sizeof cfgPass, CHARSET_TEXTO); salvarConfig(); }
      else if (item == 2) { editarTexto("IP", cfgBroker, sizeof cfgBroker, CHARSET_IP); salvarConfig(); }
      else return;
    }
  }
}

// ============================================================
//                     SENSORES E SAIDAS
// ============================================================

// Luminosidade em % (0-100), ADC de 12 bits (0-4095)
int lerLuzPct() {
  long soma = 0;
  for (int i = 0; i < 8; i++) soma += analogRead(LDR_PIN);
  int pct = map(soma / 8, 0, 4095, 0, 100);
  return LDR_INVERTIDO ? 100 - pct : pct;
}

// Aplica o estado do alerta nas saidas
void aplicarAlerta(uint8_t n) {
  alertaAtual = (n <= 3) ? n : 0;
  if (alertaAtual == 0) {
    digitalWrite(LED_PIN, LOW);
    ledcWriteTone(BUZZER_PIN, 0);
    ledLigado = false;
  }
}

// ============================================================
//                         REDE / MQTT
// ============================================================

// Chamado a cada mensagem em /TEF/<id>/cmd
void onMqtt(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.println("CMD: " + msg);

  int sep = msg.indexOf('|');
  if (msg.indexOf("@alerta") >= 0 && sep >= 0) {
    aplicarAlerta(msg.substring(sep + 1).toInt());
    mqtt.publish(topicCmdExe.c_str(), (String(DEVICE_ID) + "@alerta|ok").c_str());
  }
}

// Tenta conectar ao Wi-Fi salvo (timeout 15 s). Retorna true se conectou.
bool conectarWiFi() {
  lcdLinha(0, "Conectando WiFi");
  lcdLinha(1, cfgSsid);
  WiFi.disconnect(true);
  WiFi.begin(cfgSsid, cfgPass, WIFI_CANAL);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(250);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi OK " + WiFi.localIP().toString());
    return true;
  }
  return false;
}

void configurarBroker() {
  mqtt.disconnect();
  mqtt.setServer(cfgBroker, BROKER_PORT);
}

void tentarMqtt() {
  if (mqtt.connect(DEVICE_ID)) {
    mqtt.subscribe(topicCmd.c_str());
    Serial.println("MQTT OK");
  } else {
    Serial.println("MQTT falhou, rc=" + String(mqtt.state()));
  }
}

void publicarMedidas() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int l = lerLuzPct();
  ultL = l;
  if (isnan(t) || isnan(h)) { Serial.println("Falha no DHT"); return; }
  ultT = t; ultH = h;

  String p = "t|" + String(t, 1) + "|h|" + String(h, 0) + "|l|" + String(l);
  mqtt.publish(topicAttrs.c_str(), p.c_str());
  Serial.println("PUB: " + p);
}

// Tela principal: leituras, status do alerta e da conexao
void desenharTela() {
  char b[24];
  if (isnan(ultT)) snprintf(b, sizeof b, "T:--.-  U:--%%");
  else             snprintf(b, sizeof b, "T:%.1fC U:%d%%", ultT, (int)lround(ultH));
  lcdLinha(0, b);
  snprintf(b, sizeof b, "L:%d%% %s%s", ultL, NOME_ALERTA[alertaAtual], mqtt.connected() ? "" : " !MQ");
  lcdLinha(1, b);
}

// ============================================================
//                        SETUP / LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  ledcAttach(BUZZER_PIN, 2000, 8);   // core arduino-esp32 3.x
  dht.begin();
  lcd.init();
  lcd.backlight();

  topicAttrs  = "/TEF/" + String(DEVICE_ID) + "/attrs";
  topicCmd    = "/TEF/" + String(DEVICE_ID) + "/cmd";
  topicCmdExe = "/TEF/" + String(DEVICE_ID) + "/cmdexe";

  carregarConfig();
  mqtt.setCallback(onMqtt);
  mqtt.setSocketTimeout(3);

  // Sem rede salva: abre a configuracao. Se falhar ao conectar, volta ao menu.
  if (!cfgSsid[0]) menuConfig();
  while (!conectarWiFi()) {
    lcdLinha(0, "Falha no WiFi");
    lcdLinha(1, "Abrindo menu...");
    delay(1500);
    menuConfig();
  }
  configurarBroker();
}

void loop() {
  unsigned long agora = millis();

  // Reconexoes (sem travar a tela)
  if (WiFi.status() != WL_CONNECTED) {
    if (agora - tMqtt >= 5000) { tMqtt = agora; WiFi.reconnect(); }
  } else if (!mqtt.connected() && agora - tMqtt >= 3000) {
    tMqtt = agora;
    tentarMqtt();
  }
  mqtt.loop();

  // Telemetria
  if (agora - tPublica >= INTERVALO_PUBLICA) {
    tPublica = agora;
    publicarMedidas();
  }

  // LED pisca e buzzer acompanha, com tom e ritmo por grandeza
  if (alertaAtual != 0 && agora - tPisca >= (unsigned long)PISCA_MS[alertaAtual]) {
    tPisca = agora;
    ledLigado = !ledLigado;
    digitalWrite(LED_PIN, ledLigado);
    ledcWriteTone(BUZZER_PIN, ledLigado ? TOM_HZ[alertaAtual] : 0);
  }

  // OK abre o menu de configuracao; ao sair, reconecta com os novos dados
  if (lerBotao() == B_OK) {
    aplicarAlerta(0);
    menuConfig();
    while (!conectarWiFi()) {
      lcdLinha(0, "Falha no WiFi");
      lcdLinha(1, "Abrindo menu...");
      delay(1500);
      menuConfig();
    }
    configurarBroker();
  }

  if (agora - tTela >= 500) {
    tTela = agora;
    desenharTela();
  }
}