#include <WiFi.h>
#include <PubSubClient.h>

// ===== CONFIGURAÇÃO WI-FI (WOKWI) =====
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ===== CONFIGURAÇÃO MQTT =====
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* DEVICE_ID = "esp32_sensor_01";

// Tópicos padronizados (Seção 2.4 com prefixo Mackestacionamento)
const char* TOPIC_CATRACA = "Mackestacionamento/sensor/catraca";
const char* TOPIC_VAGAS   = "Mackestacionamento/sensor/vagas";

// ===== PINOS DOS BOTÕES =====
const int PIN_ENTRADA = 32;
const int PIN_SAIDA   = 33;
const int PIN_V1      = 25;
const int PIN_V2      = 26;
const int PIN_V3      = 27;
const int PIN_V4      = 14;

struct Botao {
  const int pino;
  const char* id;        // "entrada", "saida", "V1", "V2", "V3", "V4"
  const char* setor;     // "A", "B" ou "geral"
  bool ehVaga;
  bool estadoVaga;       // false = livre (0), true = ocupada (1)
  int ultimoEstadoLido;
  int estadoEstavel;
  unsigned long ultimoTempoDebounce;
};

const unsigned long DEBOUNCE_DELAY = 30;
unsigned long sequencia = 0; // Contador incremental (RN-01)

Botao botoes[] = {
  {PIN_ENTRADA, "entrada", "geral", false, false, HIGH, HIGH, 0},
  {PIN_SAIDA,   "saida",   "geral", false, false, HIGH, HIGH, 0},
  {PIN_V1,      "V1",      "A",     true,  false, HIGH, HIGH, 0},
  {PIN_V2,      "V2",      "A",     true,  false, HIGH, HIGH, 0},
  {PIN_V3,      "V3",      "B",     true,  false, HIGH, HIGH, 0},
  {PIN_V4,      "V4",      "B",     true,  false, HIGH, HIGH, 0}
};

const int NUM_BOTOES = sizeof(botoes) / sizeof(botoes[0]);

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

void reconectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao Broker MQTT...");
    String clientId = "Mack_Sensor_" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" conectado!");
    } else {
      Serial.printf(" falhou (rc=%d). Tentando em 3s...\n", client.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  for (int i = 0; i < NUM_BOTOES; i++) {
    pinMode(botoes[i].pino, INPUT_PULLUP);
  }

  conectarWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(120);
}

void loop() {
  if (!client.connected()) {
    reconectarMQTT();
  }
  client.loop();

  for (int i = 0; i < NUM_BOTOES; i++) {
    int leitura = digitalRead(botoes[i].pino);

    if (leitura != botoes[i].ultimoEstadoLido) {
      botoes[i].ultimoTempoDebounce = millis();
    }

    if ((millis() - botoes[i].ultimoTempoDebounce) > DEBOUNCE_DELAY) {
      if (leitura != botoes[i].estadoEstavel) {
        botoes[i].estadoEstavel = leitura;

        if (botoes[i].estadoEstavel == LOW) {
          sequencia++;
          char payload[200];

          if (botoes[i].ehVaga) {
            botoes[i].estadoVaga = !botoes[i].estadoVaga;
            int estadoNum = botoes[i].estadoVaga ? 1 : 0;

            snprintf(payload, sizeof(payload),
              "{\"dispositivo\":\"%s\",\"tipo_evento\":\"mudanca_vaga\",\"vaga\":\"%s\",\"setor\":\"%s\",\"estado\":%d,\"sequencia\":%lu}",
              DEVICE_ID, botoes[i].id, botoes[i].setor, estadoNum, sequencia);

            client.publish(TOPIC_VAGAS, payload);
            Serial.printf("[MQTT] %s -> %s\n", TOPIC_VAGAS, payload);
          } else {
            snprintf(payload, sizeof(payload),
              "{\"dispositivo\":\"%s\",\"tipo_evento\":\"%s\",\"vaga\":null,\"estado\":1,\"sequencia\":%lu}",
              DEVICE_ID, botoes[i].id, sequencia);

            client.publish(TOPIC_CATRACA, payload);
            Serial.printf("[MQTT] %s -> %s\n", TOPIC_CATRACA, payload);
          }
        }
      }
    }
    botoes[i].ultimoEstadoLido = leitura;
  }
}
