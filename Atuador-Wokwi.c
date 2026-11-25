#include <WiFi.h>
#include <PubSubClient.h>

// ===== CONFIGURAÇÃO WI-FI (WOKWI) =====
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ===== CONFIGURAÇÃO MQTT =====
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Tópicos de atuação (Seção 2.4 com prefixo Mackestacionamento)
const char* TOPIC_SETOR_A = "Mackestacionamento/atuador/setorA";
const char* TOPIC_SETOR_B = "Mackestacionamento/atuador/setorB";
const char* TOPIC_ACESSO  = "Mackestacionamento/atuador/acesso";

// ===== PINOS DOS LEDS =====
const int PIN_LUZ_A    = 18; // Iluminação Setor A
const int PIN_LUZ_B    = 19; // Iluminação Setor B
const int PIN_VERDE    = 21; // Acesso disponível
const int PIN_VERMELHO = 22; // Estacionamento lotado

// ===== CONTROLE DE ESTADO SEGURO (RN-10 / CT-07) =====
const unsigned long TIMEOUT_SEGURO_MS = 10000; // 10 segundos sem broker -> estado seguro
const unsigned long INTERVALO_RECONEXAO_MS = 3000;

unsigned long inicioDesconexao = 0;
unsigned long ultimaTentativaReconexao = 0;
bool emEstadoSeguro = false;

WiFiClient espClient;
PubSubClient client(espClient);

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado! IP: " + WiFi.localIP().toString());
}

void ativarEstadoSeguro() {
  if (!emEstadoSeguro) {
    emEstadoSeguro = true;
    digitalWrite(PIN_LUZ_A, HIGH);
    digitalWrite(PIN_LUZ_B, HIGH);
    Serial.println("[RN-10] ALERTA: Perda prolongada do broker MQTT!");
    Serial.println("[RN-10] Assumindo ESTADO SEGURO -> Iluminacao Setores A e B LIGADAS.");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();
  msg.toUpperCase();

  String topicoStr = String(topic);

  if (topicoStr == TOPIC_SETOR_A) {
    bool ligar = (msg == "1" || msg == "ON" || msg == "TRUE");
    digitalWrite(PIN_LUZ_A, ligar ? HIGH : LOW);
    Serial.printf("[LED] Setor A -> %s\n", ligar ? "LIGADO" : "DESLIGADO");
  } 
  else if (topicoStr == TOPIC_SETOR_B) {
    bool ligar = (msg == "1" || msg == "ON" || msg == "TRUE");
    digitalWrite(PIN_LUZ_B, ligar ? HIGH : LOW);
    Serial.printf("[LED] Setor B -> %s\n", ligar ? "LIGADO" : "DESLIGADO");
  } 
  else if (topicoStr == TOPIC_ACESSO) {
    if (msg == "LOTADO") {
      digitalWrite(PIN_VERDE, LOW);
      digitalWrite(PIN_VERMELHO, HIGH);
      Serial.println("[LED] Acesso -> LOTADO (Vermelho LIGADO, Verde DESLIGADO)");
    } else if (msg == "LIVRE" || msg == "DISPONIVEL") {
      digitalWrite(PIN_VERDE, HIGH);
      digitalWrite(PIN_VERMELHO, LOW);
      Serial.println("[LED] Acesso -> LIVRE (Verde LIGADO, Vermelho DESLIGADO)");
    }
  }
}

void tentarConectarMQTT() {
  Serial.print("Conectando ao Broker MQTT...");
  String clientId = "Mack_Atuador_" + String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println(" conectado!");
    emEstadoSeguro = false;
    inicioDesconexao = 0;

    client.subscribe(TOPIC_SETOR_A);
    client.subscribe(TOPIC_SETOR_B);
    client.subscribe(TOPIC_ACESSO);
    Serial.println("Topicos assinados. Aguardando comandos...");
  } else {
    Serial.printf(" falhou (rc=%d).\n", client.state());
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  pinMode(PIN_LUZ_A, OUTPUT);
  pinMode(PIN_LUZ_B, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);
  pinMode(PIN_VERMELHO, OUTPUT);

  // Estado inicial: iluminação complementar desligada e acesso livre
  digitalWrite(PIN_LUZ_A, LOW);
  digitalWrite(PIN_LUZ_B, LOW);
  digitalWrite(PIN_VERDE, HIGH);
  digitalWrite(PIN_VERMELHO, LOW);

  conectarWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(120);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    unsigned long agora = millis();

    if (inicioDesconexao == 0) {
      inicioDesconexao = agora;
    }

    // Se passar de 10s desconectado, aplica a RN-10
    if ((agora - inicioDesconexao) >= TIMEOUT_SEGURO_MS) {
      ativarEstadoSeguro();
    }

    // Tenta reconectar a cada 3s sem travar o loop
    if ((agora - ultimaTentativaReconexao) >= INTERVALO_RECONEXAO_MS) {
      ultimaTentativaReconexao = agora;
      tentarConectarMQTT();
    }
  } else {
    client.loop();
  }
}
