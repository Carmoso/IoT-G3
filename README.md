# 🚗 Mackestacionamento — Sistema de Estacionamento Inteligente (IoT)

Protótipo funcional de um **estacionamento inteligente baseado em IoT** com foco em monitoramento de ocupação em tempo real, controle de acesso automatizado e **eficiência energética** por meio de iluminação complementar sob demanda.

O projeto integra hardware simulado (ESP32 no Wokwi), mensageria MQTT, orquestração em Node-RED, persistência em banco de dados de séries temporais (InfluxDB Cloud Serverless com SQL), enriquecimento de contexto via API externa (Open-Meteo) e visualização em tempo real no Grafana Cloud.

---

## 🎯 Principais Funcionalidades

* **Monitoramento de Vagas em Tempo Real:** Detecção individual de ocupação para 4 vagas (`V1` a `V4`) com atualização instantânea de disponibilidade.
* **Controle de Fluxo (Catraca):** Contabilização acumulada de entradas e saídas de veículos.
* **Semáforo de Acesso Automatizado:** Acionamento de LEDs físico-simulados (`LIVRE` em verde / `LOTADO` em vermelho) com garantia de sincronismo via flag `Retain` no protocolo MQTT.
* **Iluminação Inteligente por Setor:** Acionamento sob demanda da iluminação complementar (`Setor A` e `Setor B`) com temporizador de **60 segundos** após movimentação, reduzindo o desperdício energético.
* **Cálculo Contínuo de Eficiência Energética:** Comparação em tempo real entre o cenário convencional (iluminação constante) e o cenário inteligente, atualizado a cada **15 segundos**.
* **Contexto Solar (API Externa):** Consulta periódica à API **Open-Meteo** (coordenadas de São Paulo) em fluxo isolado para registro de nascer do sol, pôr do sol e período (`dia`/`noite`), sem interferir na malha local de atuação.

---

## 🏗️ Arquitetura e Stack Tecnológica

```text
[ESP32 1 - Sensores (Wokwi)] ──(MQTT)──▶ [HiveMQ Broker] ──▶ [Node-RED (Orquestrador)]
                                                                 │       │        ▲
[ESP32 2 - Atuadores (Wokwi)] ◀─(MQTT)── [HiveMQ Broker] ◀───────┘       │        │ (HTTP GET)
                                                                         ▼        │
                                                    [InfluxDB Cloud Serverless]  [API Open-Meteo]
                                                                 │
                                                             (SQL API)
                                                                 ▼
                                                          [Grafana Cloud]
```

| Camada | Tecnologia / Serviço | Papel no Projeto |
| :--- | :--- | :--- |
| **Hardware (Simulado)** | 2x ESP32 (Wokwi) | **ESP32 1:** Sensores das 4 vagas e catraca.<br>**ESP32 2:** Atuadores de iluminação (Setores A/B) e semáforo de acesso. |
| **Mensageria** | MQTT (HiveMQ Public Cloud) | Transporte leve de telemetria e comandos (`broker.hivemq.com:1883`). |
| **Orquestração** | Node-RED | Regras de negócio, `trigger` de 60s, cálculo de kWh (`tick` de 15s) e consumo da API Open-Meteo. |
| **API Externa** | Open-Meteo | Contexto solar de São Paulo (`sunrise`, `sunset`, `is_day`). |
| **Banco de Dados** | InfluxDB Cloud Serverless (AWS) | Persistência de séries temporais no bucket `estacionamento_inteligente` com consultas em **SQL**. |
| **Visualização** | Grafana Cloud | Dashboard unificado com 7 painéis divididos em 2 seções (*Vagas* e *Eficiência Energética e Contexto*). |

---

## 🗄️ Modelo de Dados (InfluxDB)

Os dados são persistidos no bucket `estacionamento_inteligente` em 5 medições (*measurements*):

1. **`eventos_vagas`:** Estado individual por vaga (`vaga`, `ocupada`) e totais (`vagas_disponiveis`, `vagas_ocupadas`).
2. **`fluxo_catraca`:** Contadores acumulados (`entradas_acum`, `saidas_acum`).
3. **`iluminacao`:** Tempo acumulado de iluminação complementar ligada por setor (`setor`, `tempo_ligado_s`).
4. **`energia_estimada`:** Consumo acumulado por cenário (`cenario`: `convencional` / `inteligente`), `energia_kwh_acumulada`, `economia_kwh` e `economia_pct`.
5. **`contexto_api`:** Dados solares da Open-Meteo (`fonte`, `nascer_sol`, `por_sol`, `classe_dia_noite`).

---

## 📊 Resultados Obtidos nos Testes

Durante a validação ponta a ponta da prova de conceito, o sistema apresentou os seguintes indicadores:

* **Economia de energia em repouso:** **80,0%**
* **Economia de energia sob operação (com acionamentos reais):** **77,6% a 78,0%**
* **Energia economizada no ciclo monitorado:** **0,206 kWh**
* **Tempo de iluminação complementar registrado:** **149 s** (Setor A) e **85,4 s** (Setor B)
* **Confiabilidade de atuação:** Entrega garantida do comando `"LOTADO"`/`"LIVRE"` no tópico `Mackestacionamento/atuador/acesso` com uso da flag `Retain`.

---

## 📂 Estrutura Sugerida do Repositório

```text
├── esp32-sensores/        # Código C++ (.ino) e diagram.json do ESP32 1 (Wokwi)
├── esp32-atuadores/       # Código C++ (.ino) e diagram.json do ESP32 2 (Wokwi)
├── node-red/              # Fluxo exportado (flows.json) com regras e integração InfluxDB/Open-Meteo
├── grafana/               # JSON do dashboard unificado e queries SQL
├── docs/                  # Artigo acadêmico, diagramas e capturas de tela
└── README.md
```

---

## 🚀 Como Reproduzir o Projeto

1. **Simulação (Wokwi):**
   * Abra os projetos `esp32-sensores` e `esp32-atuadores` no [Wokwi](https://wokwi.com/) e inicie ambas as simulações.
2. **Orquestração (Node-RED):**
   * Importe o arquivo `node-red/flows.json` na sua instância do Node-RED.
   * Configure as credenciais do seu bucket `estacionamento_inteligente` (URL, Token e Organização) nos 5 nós `influxdb out`.
   * Clique em **Deploy** e dispare o `inject` da Open-Meteo para a primeira leitura solar.
3. **Visualização (Grafana):**
   * Configure o Data Source do **InfluxDB** utilizando **SQL** como linguagem de consulta.
   * Importe o dashboard em `grafana/` (as consultas SQL já estão configuradas sem a macro `$__timeFilter` e utilizam a transformação `Partition by values` para segmentação por tags).
   * Ative o *auto-refresh* (ex.: `5s`) e interaja com os botões no Wokwi para acompanhar as atualizações em tempo real.

---

## 👨‍💻 Autores

Desenvolvido como projeto acadêmico no curso de **Sistemas de Informação — Universidade Presbiteriana Mackenzie**.

* **Eduardo Carmo** ([@Carmoso](https://github.com/Carmoso))
* **Ana Clara Soares** ([@Aclssilva](https://github.com/Aclssilva))
* **Isadora Piloto** ([@Dora136](https://github.com/Dora136))
