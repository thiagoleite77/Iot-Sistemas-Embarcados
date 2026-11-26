
/*
 * ==========================================================
 * PROJETO: DATA LOGGER IoT
 * DISCIPLINA: Sistemas Embarcados
 * CURSO: Engenharia de Computação - 5º Semestre
 *
 * INTEGRANTES:
 * Thiago Leite
 * João Paulo
 * Andreo Sampaio
 * Italo Silva
 *
 * DESCRIÇÃO:
 * Firmware de referência para monitoramento ambiental,
 * comunicação MQTT e acionamento de alertas.
 *
 * PLATAFORMA:
 * ESP32 DevKit V1 - Arduino ESP32 Core 3.x
 * ==========================================================
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Importa as configurações locais, caso existam.

#if __has_include("config.local.h")
  #include "config.local.h"
#else
  #include "config.example.h"
#endif

// ==========================================================
// 1. DEFINIÇÃO DOS PINOS
// ==========================================================

constexpr uint8_t PIN_DHT = 4;
constexpr uint8_t PIN_LDR = 34;
constexpr uint8_t PIN_LED = 2;
constexpr uint8_t PIN_BUZZER = 25;

// ==========================================================
// 2. CONFIGURAÇÕES DE TEMPO
// ==========================================================

// Intervalo de publicação das medições: 5 segundos.
constexpr uint32_t INTERVALO_LEITURA = 5000;

// Intervalo entre tentativas de reconexão.
constexpr uint32_t INTERVALO_RECONEXAO = 5000;

// ==========================================================
// 3. CALIBRAÇÃO DO SENSOR LDR
// ==========================================================

// Ajustar conforme as medições do circuito físico.
// Caso a leitura esteja invertida, inverter os valores.

constexpr int ADC_ESCURO = 0;
constexpr int ADC_CLARO = 4095;

// ==========================================================
// 4. INICIALIZAÇÃO DOS OBJETOS
// ==========================================================

DHT dht(PIN_DHT, DHT11);

WiFiClient rede;

PubSubClient mqtt(rede);

// ==========================================================
// 5. TÓPICOS MQTT
// ==========================================================

// Exemplo: /TEF/datalogger001

const String base =
  String("/") + APIKEY + "/" + DEVICE_ID;

// Publicação das medições ambientais.
const String topicoAttrs = base + "/attrs";

// Recebimento dos comandos.
const String topicoCmd = base + "/cmd";

// Confirmação da execução dos comandos.
const String topicoCmdExe = base + "/cmdexe";

// ==========================================================
// 6. VARIÁVEIS DE CONTROLE
// ==========================================================

uint32_t ultimaLeitura = 0;
uint32_t ultimaWifi = 0;
uint32_t ultimaMqtt = 0;
uint32_t inicioAlerta = 0;

// Código do alerta atualmente ativo.
uint8_t alerta = 0;

bool buzzerDisponivel = false;
bool wifiTentado = false;
bool mqttTentado = false;

// ==========================================================
// 7. PADRÕES DE ALERTA
// ==========================================================
//
// Código 0: normal / nenhum alerta
// Código 1: temperatura
// Código 2: umidade
// Código 3: luminosidade
//
// Os valores representam padrões de referência.
// Devem ser confirmados com o hardware real.
// ==========================================================

struct Padrao {

  uint16_t ligadoMs;
  uint16_t desligadoMs;
  uint16_t hz;

};

constexpr Padrao padroes[] = {

  {0,   0,   0},
  {200, 200, 2000},
  {400, 400, 1500},
  {700, 700, 1000}

};

// ==========================================================
// 8. CONTROLE DO LED E BUZZER
// ==========================================================
//
// Utiliza millis() para controlar os intervalos.
// Evita delay() durante o acionamento dos alertas,
// permitindo manter a comunicação MQTT.
// ==========================================================

void atualizaSaidas() {

  const Padrao p = padroes[alerta];

  bool ativo =
    alerta != 0 &&
    (
      (millis() - inicioAlerta) %
      (p.ligadoMs + p.desligadoMs)
      < p.ligadoMs
    );

  static int ultimoLed = -1;
  static int ultimoTom = -1;

  // Atualiza o LED somente quando necessário.
  if (ultimoLed != int(ativo)) {

    digitalWrite(
      PIN_LED,
      ativo ? HIGH : LOW
    );

    ultimoLed = ativo;

  }

  // Define a frequência do buzzer.
  int tom = ativo ? p.hz : 0;

  // Atualiza a saída PWM do buzzer.
  if (
    buzzerDisponivel &&
    ultimoTom != tom
  ) {

    ledcWriteTone(PIN_BUZZER, tom);

    ultimoTom = tom;

  }

}

// ==========================================================
// 9. RECEBIMENTO DOS COMANDOS MQTT
// ==========================================================
//
// Formato UltraLight:
//
// datalogger001@alerta|0
// datalogger001@alerta|1
// datalogger001@alerta|2
// datalogger001@alerta|3
//
// ==========================================================

void recebeComando(
  char* topic,
  byte* payload,
  unsigned int length
) {

  // Verifica o tópico recebido.
  if (String(topic) != topicoCmd) {

    return;

  }

  String msg;

  // Converte os bytes recebidos em texto.
  for (
    unsigned int i = 0;
    i < length;
    ++i
  ) {

    msg += char(payload[i]);

  }

  const String prefixo =
    String(DEVICE_ID) + "@alerta|";

  // Valida o dispositivo e o comando.
  if (!msg.startsWith(prefixo)) {

    return;

  }

  // Extrai o código do alerta.
  String valor =
    msg.substring(prefixo.length());

  // Aceita somente valores entre 0 e 3.
  if (
    valor.length() != 1 ||
    valor[0] < '0' ||
    valor[0] > '3'
  ) {

    Serial.println(
      "Codigo de alerta invalido; ignorado"
    );

    return;

  }

  uint8_t novo = valor[0] - '0';

  // Atualiza o padrão quando o alerta mudar.
  if (novo != alerta) {

    alerta = novo;

    inicioAlerta = millis();

  }

  atualizaSaidas();

  // Confirma o recebimento e acionamento
  // do comando para o FIWARE.

  const String confirmacao =
    prefixo + "ok";

  if (
    !mqtt.publish(
      topicoCmdExe.c_str(),
      confirmacao.c_str()
    )
  ) {

    Serial.println(
      "Falha ao publicar confirmacao"
    );

  }

  Serial.println(msg);

}

// ==========================================================
// 10. CONEXÃO WI-FI E MQTT
// ==========================================================
//
// Controla as tentativas de reconexão sem utilizar
// laços de espera infinitos.
//
// A operação mqtt.connect() ainda pode bloquear
// temporariamente durante uma tentativa.
// ==========================================================

void conecta() {

  uint32_t agora = millis();

  // Verifica conexão Wi-Fi.
  if (WiFi.status() != WL_CONNECTED) {

    if (
      !wifiTentado ||
      agora - ultimaWifi >= INTERVALO_RECONEXAO
    ) {

      wifiTentado = true;

      ultimaWifi = agora;

      WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
      );

    }

    return;

  }

  wifiTentado = false;

  // Verifica se MQTT já está conectado.
  if (mqtt.connected()) {

    mqttTentado = false;

    return;

  }

  // Respeita o intervalo entre tentativas.
  if (
    mqttTentado &&
    agora - ultimaMqtt < INTERVALO_RECONEXAO
  ) {

    return;

  }

  mqttTentado = true;

  ultimaMqtt = agora;

  // Cria um identificador MQTT para o dispositivo.
  String clientId =
    String(DEVICE_ID) +
    "-" +
    String(
      (uint32_t)ESP.getEfuseMac(),
      HEX
    );

  // Tenta conexão com o broker.
  if (mqtt.connect(clientId.c_str())) {

    // Inscreve-se no tópico de comandos.
    if (!mqtt.subscribe(topicoCmd.c_str())) {

      mqtt.disconnect();

      return;

    }

    Serial.println("MQTT conectado");

  } else {

    Serial.printf(
      "Falha MQTT: %d\n",
      mqtt.state()
    );

  }

}

// ==========================================================
// 11. LEITURA E PUBLICAÇÃO DOS SENSORES
// ==========================================================
//
// Formato UltraLight:
//
// t|25.4|h|60.0|l|75
//
// t = temperatura
// h = umidade relativa
// l = luminosidade relativa
//
// ==========================================================

void publicaLeituras() {

  // Leitura de temperatura em Celsius.
  float t = dht.readTemperature();

  // Leitura de umidade relativa.
  float h = dht.readHumidity();

  // Converte a leitura analógica do LDR
  // para uma escala relativa de 0 a 100%.

  int l = constrain(

    map(
      analogRead(PIN_LDR),
      ADC_ESCURO,
      ADC_CLARO,
      0,
      100
    ),

    0L,
    100L

  );

  String mensagem;

  // Verifica se o DHT11 retornou valores válidos.
  if (!isnan(t) && !isnan(h)) {

    mensagem =
      "t|" + String(t, 1) +
      "|h|" + String(h, 1) +
      "|";

  } else {

    Serial.println(
      "DHT11: leitura invalida"
    );

  }

  // O LDR continua publicando mesmo
  // quando o DHT11 apresentar falha.
  mensagem += "l|" + String(l);

  // Publicação no Broker MQTT.
  if (
    mqtt.connected() &&
    mqtt.publish(
      topicoAttrs.c_str(),
      mensagem.c_str()
    )
  ) {

    Serial.println(mensagem);

  } else {

    Serial.println(
      "Telemetria nao publicada "
      "(sem conexao ou falha)"
    );

  }

}

// ==========================================================
// 12. CONFIGURAÇÃO INICIAL
// ==========================================================

void setup() {

  // Inicializa a comunicação serial.
  Serial.begin(115200);

  // Configura o LED como saída.
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, LOW);

  // Configura resolução analógica de 12 bits.
  analogReadResolution(12);

  // Inicializa o DHT11.
  dht.begin();

  // Inicializa PWM do buzzer.
  // Compatível com Arduino ESP32 Core 3.x.
  buzzerDisponivel =
    ledcAttach(
      PIN_BUZZER,
      2000,
      8
    );

  if (!buzzerDisponivel) {

    Serial.println(
      "Falha na inicializacao do buzzer PWM"
    );

  }

  atualizaSaidas();

  // Configura Wi-Fi no modo estação.
  WiFi.mode(WIFI_STA);

  // Configura o Broker MQTT.
  mqtt.setServer(
    MQTT_BROKER,
    MQTT_PORT
  );

  // Define a função responsável pelos comandos.
  mqtt.setCallback(recebeComando);

  // Configura timeout do MQTT.
  mqtt.setSocketTimeout(2);

  // Configura tamanho do buffer MQTT.
  mqtt.setBufferSize(512);

}

// ==========================================================
// 13. LOOP PRINCIPAL
// ==========================================================

void loop() {

  // Controla LED e buzzer.
  atualizaSaidas();

  // Mantém a comunicação com a infraestrutura IoT.
  conecta();

  // Processa as mensagens MQTT recebidas.
  if (mqtt.connected()) {

    mqtt.loop();

  }

  // Realiza medições periodicamente.
  if (
    millis() - ultimaLeitura
    >= INTERVALO_LEITURA
  ) {

    ultimaLeitura = millis();

    publicaLeituras();

  }

}
