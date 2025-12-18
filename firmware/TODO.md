# TODO - Melhorias Firmware Aguada IIoT

## 🎯 Roadmap de Implementação

### Fase 1 - Confiabilidade e Redundância (Imediato)

- [x] **#2 - Redundância de Gateways com Failover** ✅ IMPLEMENTADO
  - ✅ Suporta array de até 3 MACs de gateway
  - ✅ Round-robin com preferência pelo último bem-sucedido
  - ✅ Failover automático com retry exponencial
  - ✅ Persistência em NVS do gateway preferido
  - ✅ Logs detalhados de tentativas e failover
  - 📝 Arquivo: `node_ultra1/main/node_ultra1.cpp` (linhas 75-220)
  
- [x] **#7 - Protocolo ACK Bidirecional** ✅ IMPLEMENTADO
  - ✅ Estrutura `AckPacket` com magic byte, versão e status
  - ✅ Gateway envia ACK imediatamente ao receber pacote
  - ✅ Auto-registro de nós como peers no gateway
  - ✅ Nó espera ACK com timeout de 500ms
  - ✅ Retry exponencial se ACK não recebido
  - ✅ Métricas de taxa de sucesso (successful/total)
  - ✅ Logs detalhados com taxa de sucesso em tempo real
  - 📝 Arquivos: `common/telemetry_packet.h`, `node_ultra1/main/node_ultra1.cpp` (linhas 95-245), `gateway_devkit_v1/main/main.c` (linhas 187-230)

- [ ] **#5 - Health Monitoring e Diagnóstico**
  - Adicionar campos de saúde ao pacote:
    - `uptime_hours`: tempo desde último boot
    - `battery_percent`: nível de bateria (0-100)
    - `signal_quality`: % de ACKs recebidos
    - `error_count`: falhas acumuladas de sensor
    - `free_heap`: memória disponível
  - Gateway detecta nós "doentes" e alerta

### Fase 2 - Persistência e Configuração (Curto Prazo)

- [x] **#3 - Fila Local com Persistência em NVS** ✅ IMPLEMENTADO
  - ✅ Gateway armazena até 50 pacotes em NVS circular buffer
  - ✅ Fila persiste através de reboots
  - ✅ Ao voltar online, envia backlog completo ao backend
  - ✅ Flag `is_backlog` no JSON para diferenciar dados históricos
  - ✅ HTTP worker processa backlog antes de telemetria em tempo real
  - 📝 Arquivo: `gateway_devkit_v1/main/main.c` (linhas 147-225, 571-608)

- [x] **#6 - Sincronização de Tempo (SNTP)** ✅ IMPLEMENTADO
  - ✅ Gateway sincroniza com pool.ntp.br e a.st1.ntp.br
  - ✅ Usa `esp_sntp.h` do ESP-IDF 6.1
  - ✅ Timestamps em UNIX epoch (segundos desde 1970)
  - ✅ Timezone configurado: Brasil (BRT3 = UTC-3)
  - ✅ Fallback para milliseconds-since-boot se SNTP não sincronizado
  - 📝 Arquivo: `gateway_devkit_v1/main/main.c` (linhas 126-143, 232-242)

- [x] **#21 - Pacote Genérico com Tamanho Variável** ✅ IMPLEMENTADO
  - ✅ Estrutura flexível com pares chave-valor (label:type:value)
  - ✅ Suporta 9 tipos de dados: int8/16/32, uint8/16/32, float, bool, string
  - ✅ Tamanho variável: 20-250 bytes (limite ESP-NOW)
  - ✅ Até 10 pares por pacote recomendado
  - ✅ Helper functions inline para construção eficiente
  - ✅ Magic byte 0xDA para identificação
  - ✅ Ideal para prototipagem e sensores heterogêneos
  - 📝 Arquivos: `common/telemetry_packet.h`, `GENERIC_PACKET_EXAMPLE.md`, `common/generic_packet_examples.cpp`

- [x] **#22 - Pacote aguadaUltrasonic01 Ultra-Minimal** ✅ IMPLEMENTADO
  - ✅ Apenas 13 bytes (54% menor que SensorPacketV1)
  - ✅ Node envia APENAS: node_id + distance_cm
  - ✅ Servidor mantém configuração de cada node_id (MAC, offsets, geometria do tanque)
  - ✅ Servidor calcula: level_cm, percentual, volume_l
  - ✅ Servidor detecta anomalias baseado em histórico
  - ✅ Magic byte 0xA1 para identificação
  - ✅ Flags: low_battery, sensor_error
  - ✅ Calibração remota sem reflash (altera configuração no BD)
  - ✅ Ideal para redes com 50+ sensores ou operação a bateria
  - ⚠️ Requer backend funcional (não funciona offline)
  - 📝 Arquivos: `common/telemetry_packet.h`, `AGUADA_ULTRA01_PACKET.md`
  - 📝 Tabelas BD: `node_configs` (configuração), `telemetry_processed` (dados processados)

- [ ] **#4 - Calibração e Configuração Remota**
  - Novo tipo de pacote: `ConfigPacket` (gateway → nó)
  - Comandos:
    - `SET_INTERVAL`: mudar frequência de leitura (15s-3600s)
    - `SET_OFFSET`: ajustar `SENSOR_OFFSET_CM`
    - `SET_VOL_MAX`: ajustar `VOL_MAX_L` e `LEVEL_MAX_CM`
    - `REBOOT`: reiniciar nó remotamente
    - `ENABLE_DEEP_SLEEP`: ativar/desativar modo sleep
    - `FACTORY_RESET`: restaurar defaults
  - Nó persiste configurações em NVS
  - Backend envia comando via API → Gateway → Nó

### Fase 3 - Qualidade de Dados (Médio Prazo)

- [x] **#11 - Filtro Kalman Simplificado** ✅ IMPLEMENTADO
  - ✅ Substitui `median3()` por filtro Kalman 1D
  - ✅ Estima nível real com base em histórico
  - ✅ Reduz ruído de ±1cm para ±0.3cm (estimativa)
  - ✅ Parâmetros: process_noise=1.0, measurement_noise=2.0
  - ✅ Reset automático em caso de falha total do sensor
  - 📝 Arquivos: `components/ultrasonic01/ultrasonic01.h` (linhas 61-115), `node_ultra1/main/node_ultra1.cpp` (linhas 533-567)

- [ ] **#12 - Compensação de Temperatura**
  - Adicionar sensor DHT22 ou DS18B20
  - Corrigir velocidade do som: `v = 331.3 + 0.606*T`
  - Armazenar temperatura no pacote
  - Precisão melhora de ±2% para ±0.5%

- [x] **#8 - Detecção de Anomalias (Edge AI)** ✅ IMPLEMENTADO
  - ✅ Detecta mudanças bruscas: `delta >= 50cm` em intervalo de medição
  - ✅ Casos de uso implementados:
    - `ALERT_RAPID_DROP` (1): Vazamento rápido (nível cai ≥50cm)
    - `ALERT_RAPID_RISE` (2): Bomba quebrada / inundação (nível sobe ≥50cm)
    - `ALERT_SENSOR_STUCK` (3): Sensor travado (sem mudança ≥2cm por 120 minutos)
  - ✅ Campos adicionados ao `SensorPacketV1`: `flags`, `alert_type`
  - ✅ Gateway exibe alertas com destaque visual no log
  - ✅ JSON serialization inclui campos `flags` e `alert_type`
  - 📝 Arquivos: `common/telemetry_packet.h` (linhas 8-35), `node_ultra1/main/node_ultra1.cpp` (linhas 68-71, 107-110, 628-667), `gateway_devkit_v1/main/main.c` (linhas 407-438)

- [x] **#13 - Multi-Sensor Fusion** ✅ IMPLEMENTADO (node_cie_dual)
  - ✅ Suporte para 2 sensores HC-SR04 no mesmo ESP32
  - ✅ Caso de uso: Cisterna CIE com 2 reservatórios lado a lado
  - ✅ Sensor 1: GPIO1/GPIO0 → node_id=4 (CIE1)
  - ✅ Sensor 2: GPIO3/GPIO2 → node_id=5 (CIE2)
  - ✅ Leitura e envio independente (2 pacotes por ciclo)
  - ✅ Filtro Kalman independente por sensor
  - ✅ Detecção de anomalias individual
  - ✅ Tratamento de erro robusto: um sensor pode falhar, outro continua
  - ✅ Sequências NVS separadas (seq1, seq2)
  - ✅ Delay inter-sensor (100ms) para evitar interferência GPIO
  - ✅ Configuração backend SQL com capacidade real (245.000L)
  - ✅ Estratégia de MAC diferenciado (real + fictício)
  - ✅ Design de dashboard compacto com indicadores visuais
  - 📝 Firmware: `node_cie_dual/main/node_cie_dual.cpp`
  - 📝 Documentação: `node_cie_dual/README.md`
  - 📝 Backend SQL: `node_cie_dual/backend_config.sql`
  - 📝 Dashboard: `node_cie_dual/DASHBOARD_DESIGN.md`

### Fase 4 - OTA e Comissionamento (Médio Prazo)

- [ ] **#9 - OTA Updates via ESP-NOW**
  - Gateway baixa firmware do backend (HTTP)
  - Envia em chunks de 512 bytes via ESP-NOW
  - Nó grava em partição OTA
  - Validação SHA256 antes de boot
  - Rollback automático se novo firmware falhar

- [ ] **#10 - Modo de Comissionamento Automático**
  - Nó novo entra em modo discovery por 5 minutos
  - LED pisca padrão especial (SOS Morse)
  - Gateway detecta nó desconhecido
  - Envia configuração completa via ESP-NOW:
    - Channel, node_id, sensor_offset, vol_max
  - Nó persiste config e reinicia em modo normal
  - Backend API para adicionar/remover nós

### Fase 5 - Segurança (Longo Prazo)

- [ ] **#14 - Criptografia ESP-NOW com PMK**
  - Habilitar Primary Master Key (256 bits)
  - Chave única por instalação (não hardcoded)
  - Gateway e nós compartilham chave via provisioning
  - Impede spoofing e eavesdropping

- [ ] **#15 - Autenticação de Nós (Whitelist)**
  - Gateway mantém lista de MACs autorizados
  - Rejeita pacotes de dispositivos desconhecidos
  - Backend gerencia whitelist via API
  - Log de tentativas de acesso não autorizado

### Fase 6 - Arquitetura Avançada (Longo Prazo)

- [ ] **#18 - Mesh Networking (ESP-MESH)**
  - Migrar de ESP-NOW para ESP-MESH
  - Nós distantes se conectam via nós intermediários
  - Auto-healing: roteamento se adapta a falhas
  - Gateway como root node
  - Escala para 100+ nós

- [ ] **#19 - Edge Analytics no Gateway**
  - Gateway calcula:
    - Médias horárias/diárias
    - Taxa de variação (L/h)
    - Previsão de esvaziamento (horas restantes)
  - Envia apenas agregados ao backend
  - Reduz tráfego HTTP em 80-90%

- [ ] **#20 - Modo Offline com Cartão SD**
  - Gateway grava dados em SD card se backend offline
  - Formato CSV ou binário comprimido
  - Sincronização automática ao reconectar
  - Capacidade: 30 dias de dados (32GB card)

### Fase 7 - Interface e UX (Opcional)

- [ ] **#16 - Display OLED nos Nós**
  - SSD1306 128x64 I2C
  - Mostrar:
    - Nível atual (cm e %)
    - Bateria (ícone)
    - Status gateway (conectado/offline)
    - Último envio (Xs atrás)
  - Botão para acordar display (auto-sleep 30s)

- [ ] **#17 - LED RGB Multi-Estado**
  - Substituir LED simples por WS2812B
  - Estados:
    - 🟢 Verde: operação normal
    - 🟡 Amarelo: bateria < 20%
    - 🔴 Vermelho: erro de sensor
    - 🔵 Azul pulsante: enviando dados
    - 🟣 Roxo: recebendo configuração
    - ⚪ Branco piscando: modo discovery

---

## 📊 Protocolos IoT Padronizados - Análise

### Opção 1: **MQTT-SN (MQTT for Sensor Networks)**
```cpp
// Protocolo binário sobre ESP-NOW
// Mais leve que MQTT tradicional
// Tópicos curtos: "s/123/level" (sensor 123, nível)
// QoS 0, 1, 2 (nossa implementação seria QoS 1)
```
**Vantagens:**
- ✅ Padrão OASIS reconhecido
- ✅ Suporta QoS e retenção de mensagens
- ✅ Bibliotecas existentes (ex: Paho MQTT-SN)

**Desvantagens:**
- ❌ Overhead maior que protocolo custom
- ❌ Complexidade de implementação

### Opção 2: **CoAP (Constrained Application Protocol)**
```cpp
// REST-like sobre UDP
// Métodos: GET, POST, PUT, DELETE
// POST /sensors/123/level
// Payload: CBOR (JSON binário)
```
**Vantagens:**
- ✅ RFC 7252 (IETF Standard)
- ✅ Suporta observability (cliente recebe updates)
- ✅ Integração fácil com backend HTTP

**Desvantagens:**
- ❌ Requer UDP sobre WiFi (não funciona direto com ESP-NOW)
- ❌ Mais pesado que necessário

### Opção 3: **Matter (anteriormente CHIP)**
```cpp
// Protocolo de automação residencial
// Baseado em IPv6, Thread, BLE
// Interoperável com Google Home, Alexa, HomeKit
```
**Vantagens:**
- ✅ Futuro da IoT doméstica/industrial
- ✅ Descoberta automática de dispositivos
- ✅ Segurança por design

**Desvantagens:**
- ❌ Complexidade massiva (stack completo)
- ❌ Requer ESP32 com mais recursos (não C3)
- ❌ Overhead desnecessário para telemetria simples

### Opção 4: **Protobuf (Protocol Buffers)**
```proto
// Manter ESP-NOW mas serializar com Protobuf
syntax = "proto3";

message SensorTelemetry {
  uint32 version = 1;
  uint32 node_id = 2;
  bytes mac = 3;
  uint32 seq = 4;
  
  int32 distance_cm = 10;
  int32 level_cm = 11;
  uint32 percentual = 12;
  uint32 volume_l = 13;
  int32 vin_mv = 14;
  
  // Extensível sem quebrar compatibilidade
  HealthMetrics health = 20;
  AlertFlags alerts = 21;
}
```
**Vantagens:**
- ✅ Compacto e eficiente
- ✅ Evolução de schema sem breaking changes
- ✅ Usado por Google, Netflix, etc
- ✅ Geração automática de código

**Desvantagens:**
- ❌ Requer biblioteca nanopb (~30KB)
- ❌ Curva de aprendizado

### Opção 5: **CBOR (RFC 8949) + COSE**
```cpp
// JSON binário + assinaturas criptográficas
// Mais compacto que JSON, mais simples que Protobuf
{
  1: 1,              // version
  2: 42,             // node_id
  3: h'AABBCCDDEEFF', // mac
  10: 123,           // distance_cm
  11: 327            // level_cm
}
// Serializado: ~40 bytes vs 60 do struct atual
```
**Vantagens:**
- ✅ RFC standard (IETF)
- ✅ 30% menor que struct packed
- ✅ Suporta assinaturas (COSE)
- ✅ Biblioteca leve (tinycbor)

**Desvantagens:**
- ❌ Parsing mais lento que struct direto

### Opção 6: **LwM2M (Lightweight M2M)**
```cpp
// Protocolo de gestão de dispositivos IoT
// Baseado em CoAP + modelo de objetos
// Object 3303: Temperature Sensor
// Object 3330: Distance Sensor (personalizado)
```
**Vantagens:**
- ✅ OMA SpecWorks standard
- ✅ Gestão de dispositivos integrada
- ✅ Usado em redes NB-IoT

**Desvantagens:**
- ❌ Complexidade alta
- ❌ Requer servidor LwM2M

---

## 🏆 Recomendação Final

### **Sugestão Híbrida: Struct Packed + Versionamento Protobuf-Style**

Manter a simplicidade do struct atual, mas melhorar extensibilidade:

```cpp
// telemetry_packet_v2.h
#pragma once

#include <stdint.h>

// Flags de tipo de pacote
typedef enum {
    PKT_TYPE_TELEMETRY = 0x01,  // Dados de sensor
    PKT_TYPE_HEALTH = 0x02,     // Status de saúde
    PKT_TYPE_ALERT = 0x03,      // Alerta crítico
    PKT_TYPE_CONFIG = 0x10,     // Comando de configuração
    PKT_TYPE_ACK = 0x20         // Confirmação
} PacketType;

// Flags de estado
typedef enum {
    FLAG_NONE = 0x00,
    FLAG_LOW_BATTERY = 0x01,
    FLAG_SENSOR_ERROR = 0x02,
    FLAG_RAPID_CHANGE = 0x04,
    FLAG_IS_BACKLOG = 0x08,     // Dado recuperado de fila
    FLAG_COMPRESSED = 0x10      // Payload comprimido (futura)
} PacketFlags;

// Header comum (todos os pacotes)
typedef struct __attribute__((packed)) {
    uint8_t  magic;         // 0xAA (validação)
    uint8_t  version;       // = 2
    uint8_t  type;          // PacketType
    uint8_t  flags;         // PacketFlags (bitfield)
    uint8_t  node_id;       
    uint8_t  mac[6];        
    uint32_t seq;           
    uint16_t payload_len;   // Tamanho do payload (flexível)
    uint16_t checksum;      // CRC16 do payload
} PacketHeader;  // 18 bytes

// Payload de aguada_reservatorio (tipo 0x01)
typedef struct __attribute__((packed)) {
    int16_t  distance_cm;
    int16_t  level_cm;
    uint8_t  percentual;
    uint32_t volume_l;
    int16_t  vin_mv;
    
    // Timestamp (epoch UNIX ou millis)
    uint32_t timestamp;
    
    // RSSI (preenchido por gateway)
    int8_t   rssi;
} TelemetryPayload;  // 18 bytes

// Payload de saúde (tipo 0x02)
typedef struct __attribute__((packed)) {
    uint16_t uptime_hours;
    uint8_t  battery_percent;
    uint8_t  signal_quality;    // 0-100%
    uint16_t error_count;
    uint16_t success_count;
    uint32_t free_heap;
    int8_t   temperature_c;     // Temperatura interna ESP32
} HealthPayload;  // 14 bytes

// Payload de alerta (tipo 0x03)
typedef struct __attribute__((packed)) {
    uint8_t  alert_type;        // 1=vazamento, 2=bomba, 3=sensor
    int16_t  delta_cm;          // Variação que causou alerta
    uint32_t timestamp;
    char     message[32];       // Descrição curta
} AlertPayload;  // 39 bytes

// Payload de configuração (tipo 0x10, gateway → nó)
typedef struct __attribute__((packed)) {
    uint8_t  command;           // CMD_SET_INTERVAL, etc
    uint32_t value;             // Valor do parâmetro
    uint8_t  reserved[8];       // Expansão futura
} ConfigPayload;  // 13 bytes

// Payload de ACK (tipo 0x20)
typedef struct __attribute__((packed)) {
    uint32_t ack_seq;           // Sequência do pacote confirmado
    uint8_t  status;            // 0=OK, 1=erro, 2=retry
    int8_t   rssi;              // RSSI medido pelo receptor
} AckPayload;  // 6 bytes

// Pacote completo
typedef struct __attribute__((packed)) {
    PacketHeader header;
    union {
        TelemetryPayload telemetry;
        HealthPayload health;
        AlertPayload alert;
        ConfigPayload config;
        AckPayload ack;
        uint8_t raw[250];       // Máximo ESP-NOW = 250 bytes
    } payload;
} SensorPacket;

// Funções auxiliares
static inline uint16_t packet_calc_checksum(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for(uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(uint8_t j = 0; j < 8; j++) {
            if(crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

static inline bool packet_validate(const SensorPacket *pkt) {
    if(pkt->header.magic != 0xAA) return false;
    if(pkt->header.version != 2) return false;
    uint16_t crc = packet_calc_checksum(pkt->payload.raw, pkt->header.payload_len);
    return (crc == pkt->header.checksum);
}
```

### Vantagens dessa Abordagem:
- ✅ Mantém eficiência de struct packed
- ✅ Extensível sem quebrar compatibilidade (novos tipos)
- ✅ CRC para detecção de corrupção
- ✅ Suporta múltiplos tipos de mensagem
- ✅ Pronto para futuras features (compressão, criptografia)
- ✅ Validação de integridade (magic + checksum)

---

## 🔄 Deep Sleep - Implementação

### Arquivo: `node_ultra1/main/node_ultra1.cpp`

```cpp
/* ====== DEEP SLEEP CONFIGURATION ====== */
#define DEEP_SLEEP_ENABLED_KEY "ds_enabled"  // NVS key
#define DEEP_SLEEP_DEFAULT false             // Padrão: desabilitado (desenvolvimento)

// Função para ler configuração de deep sleep
static bool is_deep_sleep_enabled(void) {
    nvs_handle_t nvs;
    uint8_t enabled = DEEP_SLEEP_DEFAULT;
    
    if(nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u8(nvs, DEEP_SLEEP_ENABLED_KEY, &enabled);
        nvs_close(nvs);
    }
    return (enabled != 0);
}

// Função para setar deep sleep via comando remoto
static void set_deep_sleep_mode(bool enable) {
    nvs_handle_t nvs;
    if(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, DEEP_SLEEP_ENABLED_KEY, enable ? 1 : 0);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "Deep sleep %s", enable ? "HABILITADO" : "DESABILITADO");
    }
}

// No final do loop principal:
void app_main(void) {
    // ... código existente ...
    
    while(1) {
        // Leitura e envio de dados
        measure_and_send();
        
        // Verificar modo de sleep
        if(is_deep_sleep_enabled()) {
            ESP_LOGI(TAG, "Entrando em deep sleep por %d segundos", SAMPLE_INTERVAL_S);
            esp_deep_sleep(SAMPLE_INTERVAL_S * 1000000ULL);
            // Após wake-up, ESP32 reinicia do zero
        } else {
            ESP_LOGI(TAG, "Deep sleep DESABILITADO - aguardando %d segundos", SAMPLE_INTERVAL_S);
            vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_S * 1000));
        }
    }
}
```

### Comandos para Habilitar/Desabilitar Deep Sleep:

**Opção 1: Via Serial (desenvolvimento)**
```cpp
// Adicionar parser de comandos serial
void handle_serial_command(const char *cmd) {
    if(strcmp(cmd, "sleep on") == 0) {
        set_deep_sleep_mode(true);
    } else if(strcmp(cmd, "sleep off") == 0) {
        set_deep_sleep_mode(false);
    } else if(strcmp(cmd, "status") == 0) {
        ESP_LOGI(TAG, "Deep sleep: %s", is_deep_sleep_enabled() ? "ON" : "OFF");
    }
}
```

**Opção 2: Via ESP-NOW (produção)**
```cpp
// Gateway envia ConfigPacket com comando ENABLE_DEEP_SLEEP
typedef enum {
    CMD_ENABLE_DEEP_SLEEP = 0x01,
    CMD_DISABLE_DEEP_SLEEP = 0x02,
    // ... outros comandos
} ConfigCommand;

// Nó recebe e processa
void handle_config_packet(const ConfigPayload *cfg) {
    switch(cfg->command) {
        case CMD_ENABLE_DEEP_SLEEP:
            set_deep_sleep_mode(true);
            break;
        case CMD_DISABLE_DEEP_SLEEP:
            set_deep_sleep_mode(false);
            break;
    }
}
```

**Opção 3: Via Backend API** (mais elegante)
```bash
# Backend envia comando ao gateway, que repassa ao nó
curl -X POST http://192.168.0.117:8080/api/nodes/1/config \
  -H "Content-Type: application/json" \
  -d '{"command": "deep_sleep", "enabled": true}'
```

---

## ⚠️ Nota sobre Deep Sleep

**IMPORTANTE:** Deep sleep deve ser **implementado por último** porque:

1. **Dificulta debugging**: Nó reinicia a cada ciclo, perde logs
2. **Quebra OTA**: Não dá tempo para receber firmware novo
3. **Impede reconfiguração**: Janela curta para enviar comandos

**Estratégia recomendada:**
- Desenvolvimento: Deep sleep OFF (default)
- Testes de campo: Deep sleep OFF + monitoramento
- Produção final: Deep sleep ON via comando remoto

**Alternativa mais flexível:**
```cpp
// Wake-up periódico mais frequente para check de comandos
#define LIGHT_SLEEP_CHECK_S 5    // Acorda a cada 5s para checar comandos
#define SAMPLES_BEFORE_DATA 6    // Envia dados a cada 30s (6 × 5s)

// Permite OTA e reconfiguração mesmo em modo sleep
```

---

## 📝 Priorização Final

### Sprint 1 (1-2 semanas):
1. ✅ Redundância de gateways (#2)
2. ✅ ACK bidirecional (#7)
3. ✅ Health monitoring (#5)

### Sprint 2 (2-3 semanas):
4. ✅ Fila persistente NVS (#3)
5. ✅ SNTP no gateway (#6)
6. ✅ Novo protocolo de pacotes (v2)

### Sprint 3 (3-4 semanas):
7. ✅ Calibração remota (#4)
8. ✅ Detecção de anomalias (#8)
9. ✅ Filtro Kalman (#11)

### Sprint 4 (1-2 meses):
10. ✅ OTA updates (#9)
11. ✅ Comissionamento automático (#10)
12. ⚠️ **Deep sleep (ÚLTIMO!)** (#1)

---

**Próximos passos:** Qual implementação você quer que eu comece? Recomendo #2 (redundância) ou #7 (ACK) como primeiro passo.
