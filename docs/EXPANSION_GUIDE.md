# Guia de Expansão e Configuração Dinâmica - Aguada Telemetry

## 1. Arquitetura de Expansão Modular

### Conceito: Telemetria Extensível

```c
// Estrutura base versátil (atual: SensorPacketV1)
typedef struct __attribute__((packed)) {
    uint8_t  version;        // Versão do protocolo
    uint8_t  node_id;        // ID do nó
    uint8_t  mac[6];         // MAC address
    uint32_t seq;            // Sequência
    
    // Campos dinâmicos por tipo de sensor
    uint8_t  sensor_type;    // 0x01=ultrasonic, 0x02=temp, 0x03=relay, etc.
    uint8_t  data_length;    // Tamanho do payload de dados
    uint8_t  data[64];       // Payload flexível (até 64 bytes)
    
    // Campos do gateway
    int8_t   rssi;
    uint32_t ts_ms;
} SensorPacketV2;
```

## 2. Estratégias de Adição de Novos Sensores

### Opção A: Protocolo Versionado (Recomendado para poucos tipos)

**Quando usar:** Até ~5 tipos diferentes de sensores

**Vantagens:**
- ✅ Pacotes compactos
- ✅ Performance máxima
- ✅ Validação forte no backend

**Desvantagens:**
- ❌ Requer reflash de todos os nós ao adicionar novo tipo
- ❌ Schema SQL fixo

**Implementação:**

```c
// common/telemetry_packet.h
#define SENSOR_PACKET_VERSION 2

// Tipos de pacote
typedef enum {
    PKT_ULTRASONIC = 0x01,
    PKT_TEMPERATURE = 0x02,
    PKT_RELAY_STATUS = 0x03,
    PKT_MULTI_SENSOR = 0x04,  // Pacote combinado
} packet_type_t;

// Pacote ultrassônico (atual)
typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  node_id;
    uint8_t  mac[6];
    uint32_t seq;
    uint8_t  packet_type;    // PKT_ULTRASONIC
    
    int16_t  distance_cm;
    int16_t  level_cm;
    uint8_t  percentual;
    uint32_t volume_l;
    int16_t  vin_mv;
    
    int8_t   rssi;
    uint32_t ts_ms;
} UltrasonicPacket;

// Novo: Pacote de temperatura
typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  node_id;
    uint8_t  mac[6];
    uint32_t seq;
    uint8_t  packet_type;    // PKT_TEMPERATURE
    
    int16_t  temp_c;         // Temperatura °C * 10 (ex: 254 = 25.4°C)
    int16_t  humidity;       // Umidade % * 10
    int16_t  pressure_hpa;   // Pressão hPa
    int16_t  vin_mv;
    
    int8_t   rssi;
    uint32_t ts_ms;
} TemperaturePacket;

// Novo: Pacote de controle (relé, LED, etc)
typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  node_id;
    uint8_t  mac[6];
    uint32_t seq;
    uint8_t  packet_type;    // PKT_RELAY_STATUS
    
    uint8_t  relay_states;   // Bitmask: bit 0=relé1, bit 1=relé2, etc
    uint8_t  led_rgb[3];     // RGB
    uint16_t button_states;  // Bitmask de botões
    int16_t  vin_mv;
    
    int8_t   rssi;
    uint32_t ts_ms;
} RelayControlPacket;
```

**Backend (PHP) - ingest_sensorpacket_v2.php:**

```php
<?php
require_once 'config.php';

$json = file_get_contents('php://input');
$data = json_decode($json, true);

if (!$data || !isset($data['packet_type'])) {
    http_response_code(400);
    die('Invalid packet');
}

$mysqli = db_connect();

switch ($data['packet_type']) {
    case 0x01: // Ultrasonic
        $stmt = $mysqli->prepare("
            INSERT INTO leituras_ultrasonic 
            (node_id, mac, seq, distance_cm, level_cm, percentual, volume_l, vin_mv, rssi, ts_ms)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ");
        $stmt->bind_param("isiiiiiii", 
            $data['node_id'], $data['mac'], $data['seq'],
            $data['distance_cm'], $data['level_cm'], $data['percentual'],
            $data['volume_l'], $data['vin_mv'], $data['rssi'], $data['ts_ms']
        );
        break;
        
    case 0x02: // Temperature
        $stmt = $mysqli->prepare("
            INSERT INTO leituras_temperature
            (node_id, mac, seq, temp_c, humidity, pressure_hpa, vin_mv, rssi, ts_ms)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ");
        $stmt->bind_param("isiiiiii",
            $data['node_id'], $data['mac'], $data['seq'],
            $data['temp_c'], $data['humidity'], $data['pressure_hpa'],
            $data['vin_mv'], $data['rssi'], $data['ts_ms']
        );
        break;
        
    case 0x03: // Relay/Control
        $stmt = $mysqli->prepare("
            INSERT INTO leituras_control
            (node_id, mac, seq, relay_states, led_rgb_r, led_rgb_g, led_rgb_b, button_states, vin_mv, rssi, ts_ms)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ");
        $stmt->bind_param("isiiiiiiiii",
            $data['node_id'], $data['mac'], $data['seq'],
            $data['relay_states'], $data['led_rgb'][0], $data['led_rgb'][1], $data['led_rgb'][2],
            $data['button_states'], $data['vin_mv'], $data['rssi'], $data['ts_ms']
        );
        break;
        
    default:
        http_response_code(400);
        die('Unknown packet type');
}

if ($stmt->execute()) {
    echo json_encode(['success' => true, 'id' => $mysqli->insert_id]);
} else {
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => $stmt->error]);
}
```

**Schema SQL:**

```sql
-- Tabela por tipo de sensor
CREATE TABLE leituras_ultrasonic (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    node_id SMALLINT NOT NULL,
    mac VARCHAR(17) NOT NULL,
    seq INT NOT NULL,
    distance_cm INT,
    level_cm INT,
    percentual TINYINT,
    volume_l INT,
    vin_mv INT,
    rssi TINYINT,
    ts_ms BIGINT,
    INDEX idx_node_ts (node_id, created_at)
);

CREATE TABLE leituras_temperature (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    node_id SMALLINT NOT NULL,
    mac VARCHAR(17) NOT NULL,
    seq INT NOT NULL,
    temp_c INT,          -- Temperatura * 10
    humidity INT,        -- Umidade * 10
    pressure_hpa INT,
    vin_mv INT,
    rssi TINYINT,
    ts_ms BIGINT,
    INDEX idx_node_ts (node_id, created_at)
);

CREATE TABLE leituras_control (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    node_id SMALLINT NOT NULL,
    mac VARCHAR(17) NOT NULL,
    seq INT NOT NULL,
    relay_states TINYINT,     -- Bitmask
    led_rgb_r TINYINT,
    led_rgb_g TINYINT,
    led_rgb_b TINYINT,
    button_states SMALLINT,   -- Bitmask
    vin_mv INT,
    rssi TINYINT,
    ts_ms BIGINT,
    INDEX idx_node_ts (node_id, created_at)
);
```

---

### Opção B: JSON Flexível (Recomendado para muitos tipos)

**Quando usar:** Mais de 5 tipos ou sensores customizados frequentes

**Vantagens:**
- ✅ Adicionar sensores sem reflash
- ✅ Configuração dinâmica
- ✅ Schema SQL flexível (JSON column)

**Desvantagens:**
- ❌ Pacotes maiores (~200 bytes vs 30 bytes)
- ❌ Parsing mais lento
- ❌ Mais tráfego ESP-NOW

**Implementação:**

```c
// Nó envia JSON compacto
const char* payload = 
    "{\"t\":\"ultra\","      // tipo
    "\"d\":123,"             // distance
    "\"l\":347,"             // level
    "\"p\":77,"              // percentual
    "\"v\":61600,"           // volume
    "\"vin\":3300}";         // voltage
```

**Schema SQL:**

```sql
CREATE TABLE leituras_flexivel (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    node_id SMALLINT NOT NULL,
    sensor_type VARCHAR(20) NOT NULL,  -- 'ultrasonic', 'temperature', etc
    data JSON NOT NULL,                -- Dados flexíveis
    rssi TINYINT,
    ts_ms BIGINT,
    INDEX idx_node_type_ts (node_id, sensor_type, created_at)
);

-- Queries com JSON
SELECT 
    node_id,
    sensor_type,
    JSON_EXTRACT(data, '$.temp_c') as temperatura,
    JSON_EXTRACT(data, '$.humidity') as umidade
FROM leituras_flexivel
WHERE sensor_type = 'temperature';
```

---

## 3. Configuração Dinâmica de Parâmetros

### Problema: Como alterar `node_id`, `sensor_offset`, etc sem reflash?

### Solução A: NVS (Non-Volatile Storage) - Recomendado

**Configuração via Serial CLI:**

```c
// node_ultra1/main/config_manager.h
#include "nvs_flash.h"
#include "nvs.h"

#define NVS_NAMESPACE "aguada_cfg"

typedef struct {
    uint8_t  node_id;
    int16_t  sensor_offset_cm;
    int32_t  vol_max_l;
    int16_t  level_max_cm;
    uint8_t  gateway_mac[6];
    uint8_t  espnow_channel;
    uint16_t sample_interval_s;
} node_config_t;

// Carregar configuração do NVS
esp_err_t config_load(node_config_t *cfg) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    
    size_t size = sizeof(node_config_t);
    err = nvs_get_blob(h, "config", cfg, &size);
    nvs_close(h);
    return err;
}

// Salvar configuração no NVS
esp_err_t config_save(const node_config_t *cfg) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    
    err = nvs_set_blob(h, "config", cfg, sizeof(node_config_t));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

// Defaults de fábrica
void config_factory_reset(node_config_t *cfg) {
    cfg->node_id = 1;
    cfg->sensor_offset_cm = 20;
    cfg->vol_max_l = 80000;
    cfg->level_max_cm = 450;
    uint8_t default_mac[] = {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84};
    memcpy(cfg->gateway_mac, default_mac, 6);
    cfg->espnow_channel = 11;
    cfg->sample_interval_s = 30;
}
```

**CLI Serial (comandos via UART):**

```c
// Adicionar ao main.c
#include "esp_console.h"
#include "argtable3/argtable3.h"

static int cmd_set_node_id(int argc, char **argv) {
    if (argc != 2) {
        printf("Uso: set_node_id <id>\n");
        return 1;
    }
    
    node_config_t cfg;
    config_load(&cfg);
    cfg.node_id = atoi(argv[1]);
    config_save(&cfg);
    
    printf("Node ID alterado para %d (reinicie para aplicar)\n", cfg.node_id);
    return 0;
}

static int cmd_set_offset(int argc, char **argv) {
    if (argc != 2) {
        printf("Uso: set_offset <cm>\n");
        return 1;
    }
    
    node_config_t cfg;
    config_load(&cfg);
    cfg.sensor_offset_cm = atoi(argv[1]);
    config_save(&cfg);
    
    printf("Sensor offset alterado para %d cm\n", cfg.sensor_offset_cm);
    return 0;
}

static int cmd_show_config(int argc, char **argv) {
    node_config_t cfg;
    if (config_load(&cfg) == ESP_OK) {
        printf("\n=== Configuração Atual ===\n");
        printf("Node ID: %d\n", cfg.node_id);
        printf("Sensor Offset: %d cm\n", cfg.sensor_offset_cm);
        printf("Volume Máximo: %d L\n", cfg.vol_max_l);
        printf("Nível Máximo: %d cm\n", cfg.level_max_cm);
        printf("Intervalo de Amostragem: %d s\n", cfg.sample_interval_s);
        printf("Canal ESP-NOW: %d\n", cfg.espnow_channel);
    } else {
        printf("Erro ao carregar configuração\n");
    }
    return 0;
}

void register_config_commands() {
    esp_console_cmd_t cmd_set_id = {
        .command = "set_node_id",
        .help = "Define o ID do nó",
        .hint = NULL,
        .func = &cmd_set_node_id,
    };
    esp_console_cmd_register(&cmd_set_id);
    
    esp_console_cmd_t cmd_offset = {
        .command = "set_offset",
        .help = "Define o offset do sensor (cm)",
        .hint = NULL,
        .func = &cmd_set_offset,
    };
    esp_console_cmd_register(&cmd_offset);
    
    esp_console_cmd_t cmd_show = {
        .command = "show_config",
        .help = "Mostra configuração atual",
        .hint = NULL,
        .func = &cmd_show_config,
    };
    esp_console_cmd_register(&cmd_show);
}
```

**Uso via terminal:**

```bash
# Conectar ao nó
screen /dev/ttyACM0 115200

# Comandos disponíveis
> show_config
> set_node_id 3
> set_offset 25
> reboot
```

---

### Solução B: Web Config (Gateway como Portal Cativo)

**Implementação no gateway:**

```c
// Gateway cria AP temporário para configuração
#include "esp_http_server.h"

// Endpoint para configurar nó via Wi-Fi
static esp_err_t config_node_handler(httpd_req_t *req) {
    char buf[100];
    int ret = httpd_req_recv(req, buf, sizeof(buf));
    
    // Parse JSON: {"node_id": 3, "offset": 25, "mac": "DC:54:75:..."}
    cJSON *json = cJSON_Parse(buf);
    uint8_t node_id = cJSON_GetObjectItem(json, "node_id")->valueint;
    uint8_t target_mac[6]; // Parse do MAC
    
    // Montar pacote de configuração
    config_packet_t cfg_pkt = {
        .type = PKT_CONFIG,
        .node_id = node_id,
        .sensor_offset = cJSON_GetObjectItem(json, "offset")->valueint,
        // ... outros parâmetros
    };
    
    // Enviar via ESP-NOW para o nó
    esp_now_send(target_mac, (uint8_t*)&cfg_pkt, sizeof(cfg_pkt));
    
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}
```

---

## 4. Workflow de Atualização Recomendado

### Cenário 1: Adicionar Sensor de Temperatura

**Passos:**

1. **Firmware (node_temperature/):**
   ```bash
   cp -r node_ultra1 node_temperature
   # Editar main.c com DHT22/BME280
   ```

2. **Protocolo (common/telemetry_packet.h):**
   ```c
   // Adicionar TemperaturePacket (versão 2)
   #define SENSOR_PACKET_VERSION 2
   ```

3. **Database (database/migrations/002_add_temperature.sql):**
   ```sql
   CREATE TABLE leituras_temperature (...);
   ```

4. **Backend (backend/ingest_sensorpacket_v2.php):**
   ```php
   // Adicionar case 0x02 para temperatura
   ```

5. **Deploy:**
   ```bash
   # Flasha novo nó
   cd node_temperature && idf.py flash
   
   # Atualiza banco
   mysql -u root -p sensores_db < database/migrations/002_add_temperature.sql
   
   # Backend automaticamente detecta packet_type
   ```

### Cenário 2: Alterar node_id de Nó Remoto

**Opção A (Serial - acesso físico):**
```bash
screen /dev/ttyACM0 115200
> set_node_id 5
> reboot
```

**Opção B (OTA - sem acesso físico):**
```bash
# Via gateway web config
curl -X POST http://gateway.local/config_node \
  -d '{"mac":"DC:54:75:XX:XX:XX", "node_id":5}'
```

**Opção C (Reflash - mais trabalhoso):**
```bash
# Editar node_ultra1/main/node_ultra1.cpp
#define NODE_ID 5

# Reflash
idf.py flash
```

---

## 5. Checklist de Compatibilidade

Ao adicionar novos sensores, verifique:

- [ ] `SENSOR_PACKET_VERSION` incrementado?
- [ ] Gateway consegue parsear novos campos?
- [ ] Backend tem endpoint/tabela para novo tipo?
- [ ] Dashboard mostra novos dados?
- [ ] Documentação atualizada (README, ARCHITECTURE.md)?
- [ ] Testes com 1 nó antes de deploy em massa?

---

## 6. Recomendações Finais

**Para seu caso (4 ultra1 + 1 ultra2 já deployados):**

1. **Curto prazo:** Use **NVS + CLI serial** para configurações (node_id, offset)
2. **Médio prazo:** Adicione **Portal Cativo no Gateway** para config remota
3. **Longo prazo:** Migre para **Opção B (JSON flexível)** se adicionar muitos tipos de sensores

**Prioridades:**
1. ✅ Implementar NVS config (1-2 dias)
2. ✅ Adicionar CLI serial (1 dia)
3. ✅ Documentar protocolo de adição de sensores (feito acima)
4. ⏳ Portal cativo no gateway (1 semana)
5. ⏳ Sistema de OTA updates (2 semanas)
