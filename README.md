# 🚗 Mackestacionamento — Sistema de Estacionamento Inteligente (IoT)

Protótipo funcional de um **estacionamento inteligente baseado em IoT** com foco em monitoramento de ocupação em tempo real, controle de acesso automatizado e **eficiência energética** por meio de iluminação complementar sob demanda.

O projeto integra hardware simulado (ESP32 no Wokwi), mensageria MQTT, orquestração em Node-RED, persistência em banco de dados de séries temporais (InfluxDB Cloud Serverless com SQL), enriquecimento de contexto via API externa (Open-Meteo) e visualização em tempo real no Grafana Cloud.

---

## 🎯 Principais Funcionalidades

* **Monitoramento de Vagas em Tempo Real:** Detecção individual de ocupação para 4 vagas (`V1` a `V4`) com atualização instantânea de disponibilidade.
* **Controle de Fluxo (Catraca):** Contabilização acumulada de entradas e saídas de veículos.
* **Semáforo de Acesso Automatizado:** Acionamento de LEDs físico-simulados (`LIVRE` em verde / `LOTADO` em vermelho) com garantia de sincronismo via flag `Retain` no protocolo MQTT.
* **Iluminação Inteligente por Setor:** Acionamento sob demanda da iluminação complementar (`Setor A` e `Setor B`) com temporizador de **60 segundos** após movimentação, reduzindo o desperdício energético.
* **Cálculo Cont
