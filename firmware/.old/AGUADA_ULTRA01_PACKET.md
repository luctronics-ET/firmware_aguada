# aguadaUltrasonic01 - Ultra-Minimal Telemetry Packet

## 🎯 Design Philosophy

O pacote `aguadaUltrasonic01` implementa a arquitetura **Edge-to-Cloud** com processamento centralizado:

- **Edge (Node)**: Coleta dado bruto (`distance_cm`) e envia
- **Gateway**: Adiciona metadados (RSSI, timestamp)
- **Server**: Processa tudo (level, percentage, volume, anomalias)

### Vantagens dessa Abordagem

✅ **Tamanho ultra-compacto**: 13 bytes (vs 28 bytes do `SensorPacketV1`)  
✅ **54% redução de banda**: Menos uso de ESP-NOW, mais eficiente  
✅ **Facilita calibração**: Alterar offsets sem reflash de nós  
✅ **Escalabilidade**: Adicionar sensores sem mudar firmware  
✅ **Centralização de lógica**: Um lugar para ajustar algoritmos  
✅ **Bateria otimizada**: Menos processamento no nó = menor consumo  

### Quando Usar

| Cenário | aguadaUltrasonic01 | SensorPacketV1 |
|---------|-------------------|----------------|
| Rede com 50+ sensores | ✅ Ideal | ❌ Overhead |
| Calibração frequente | ✅ Ideal | ❌ Requer reflash |
| Processamento edge | ❌ Não suporta | ✅ Ideal |
| Latência crítica (<1s) | ❌ Depende servidor | ✅ Ideal |
| Instalação temporária | ✅ Ideal | ⚠️ Funciona |
| Offline (sem backend) | ❌ Não funciona | ✅ Ideal |

---

## 📦 Estrutura do Pacote

### Formato Binário (13 bytes total)

```
┌──────────────────────────────────────────────────────────────┐
│  Byte 0  │  Byte 1  │  Byte 2  │ Byte 3-4 │  Byte 5  │       │
│  magic   │ version  │ node_id  │distance_ │  flags   │  ...  │
│  (0xA1)  │   (1)    │ (0-255)  │   cm     │          │       │
├──────────────────────────────────────────────────────────────┤
│  Byte 6  │  Byte 7  │ Byte 8-11                               │
│ reserved │   rssi   │   ts_ms   (gateway timestamp)           │
│          │  (dBm)   │   (uint32_t milliseconds)               │
└──────────────────────────────────────────────────────────────┘
```

### Campos Detalhados

| Campo | Tipo | Bytes | Preenchido por | Descrição |
|-------|------|-------|----------------|-----------|
| `magic` | `uint8_t` | 1 | Node | `0xA1` - Identifica pacote aguadaUltrasonic01 |
| `version` | `uint8_t` | 1 | Node | `1` - Versão do protocolo |
| `node_id` | `uint8_t` | 1 | Node | Identificador único do nó (0-255) |
| `distance_cm` | `int16_t` | 2 | Node | **DADO PRINCIPAL**: Distância medida pelo sensor (cm) |
| `flags` | `uint8_t` | 1 | Node | Bit 0: low_battery, Bit 1: sensor_error |
| `reserved` | `uint8_t` | 1 | Node | Reservado para expansão futura |
| `rssi` | `int8_t` | 1 | Gateway | Intensidade do sinal (dBm) |
| `ts_ms` | `uint32_t` | 4 | Gateway | Timestamp em milissegundos |

### Flags Disponíveis

```c
#define ULTRA01_FLAG_LOW_BATTERY  0x01  // Bit 0: Bateria baixa
#define ULTRA01_FLAG_SENSOR_ERROR 0x02  // Bit 1: Falha na leitura do sensor
// Bits 2-7: Reservados para uso futuro
```

---

## 💾 Configuração no Servidor

Cada `node_id` mapeia para uma configuração armazenada no banco de dados:

### Tabela: `node_configs`

```sql
CREATE TABLE node_configs (
    node_id TINYINT UNSIGNED PRIMARY KEY,
    mac VARCHAR(17) NOT NULL,
    location VARCHAR(32) NOT NULL,
    
    -- Tank geometry
    sensor_offset_cm SMALLINT NOT NULL DEFAULT 20,
    level_max_cm SMALLINT NOT NULL DEFAULT 450,
    vol_max_l INT UNSIGNED NOT NULL DEFAULT 80000,
    
    -- Calibration
    distance_offset_cm SMALLINT NOT NULL DEFAULT 0,
    
    -- Anomaly detection
    rapid_change_threshold_cm SMALLINT NOT NULL DEFAULT 50,
    no_change_minutes SMALLINT UNSIGNED NOT NULL DEFAULT 120,
    
    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_mac (mac)
);
```

### Exemplo de Configuração

```sql
INSERT INTO node_configs (node_id, mac, location, sensor_offset_cm, level_max_cm, vol_max_l) VALUES
(1, 'C8:2B:96:AA:BB:CC', 'Reservoir A', 20, 450, 80000),
(2, 'C8:2B:96:DD:EE:FF', 'Reservoir B', 15, 300, 50000),
(3, 'C8:2B:96:11:22:33', 'Tank C', 25, 600, 120000);
```

---

## 🔄 Fluxo de Dados

```
┌────────────┐         ┌─────────────┐         ┌──────────────┐
│   Node     │ ESP-NOW │   Gateway   │  HTTP   │    Server    │
│ (ESP32-C3) │────────▶│ (ESP32 V1)  │────────▶│ (PHP/MySQL)  │
└────────────┘         └─────────────┘         └──────────────┘
     │                        │                        │
     │ 1. Mede distance_cm    │                        │
     │ 2. Constrói pacote     │                        │
     │    (13 bytes)          │                        │
     │────────────────────────▶│                        │
     │                        │ 3. Adiciona RSSI       │
     │                        │ 4. Adiciona timestamp  │
     │                        │ 5. Serializa JSON      │
     │                        │────────────────────────▶│
     │                        │                        │ 6. Lookup node_id
     │                        │                        │ 7. Calcula level_cm
     │                        │                        │ 8. Calcula percentual
     │                        │                        │ 9. Calcula volume_l
     │                        │                        │10. Detecta anomalias
     │                        │                        │11. Grava em BD
     │                        │                        │12. Atualiza dashboard
```

---

## 📝 Implementação no Node

### Exemplo: `node_ultra1/main/node_ultra1.cpp`

```cpp
#include "telemetry_packet.h"
#include "ultrasonic01.h"

#define NODE_ID 1  // Configurar para cada nó

void send_telemetry(void) {
    // 1. Medir distância
    int distance = ultrasonic_measure_cm(TRIG_GPIO, ECHO_GPIO);
    
    // 2. Construir pacote mínimo
    aguadaUltrasonic01Packet pkt = {0};
    pkt.magic = AGUADA_ULTRA01_MAGIC;
    pkt.version = AGUADA_ULTRA01_VERSION;
    pkt.node_id = NODE_ID;
    pkt.distance_cm = (distance < 0) ? -1 : (int16_t)distance;
    
    // 3. Flags opcionais
    if (get_battery_voltage() < 3300) {
        pkt.flags |= ULTRA01_FLAG_LOW_BATTERY;
    }
    if (distance < 0) {
        pkt.flags |= ULTRA01_FLAG_SENSOR_ERROR;
    }
    
    // 4. Enviar via ESP-NOW (gateway preenche rssi e ts_ms)
    esp_now_send(gateway_mac, (uint8_t*)&pkt, sizeof(pkt));
    
    ESP_LOGI(TAG, "Sent: node_id=%d, distance=%dcm, flags=0x%02X", 
             pkt.node_id, pkt.distance_cm, pkt.flags);
}
```

---

## 🌐 Processamento no Gateway

### Exemplo: `gateway_devkit_v1/main/main.c`

```c
void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    // Detectar tipo de pacote pelo magic byte
    if (len >= 1) {
        uint8_t magic = data[0];
        
        if (magic == AGUADA_ULTRA01_MAGIC && len == sizeof(aguadaUltrasonic01Packet)) {
            aguadaUltrasonic01Packet *pkt = (aguadaUltrasonic01Packet *)data;
            
            // Gateway preenche metadados
            pkt->rssi = info->rx_ctrl->rssi;
            pkt->ts_ms = get_unix_timestamp();  // SNTP ou millis()
            
            // Serializar para JSON
            char json[256];
            snprintf(json, sizeof(json),
                "{"
                "\"packet_type\":\"aguadaUltrasonic01\","
                "\"node_id\":%d,"
                "\"distance_cm\":%d,"
                "\"flags\":%d,"
                "\"rssi\":%d,"
                "\"ts_ms\":%u"
                "}",
                pkt->node_id,
                pkt->distance_cm,
                pkt->flags,
                pkt->rssi,
                pkt->ts_ms
            );
            
            // Enviar ao backend
            http_post_json(INGEST_URL, json);
            
            ESP_LOGI(TAG, "Forwarded aguadaUltrasonic01: node=%d, dist=%dcm",
                     pkt->node_id, pkt->distance_cm);
        }
        else if (magic == 0x01) {  // SensorPacketV1 (sem magic, version=1)
            // Processar pacote v1 existente
            // ...
        }
        else if (magic == 0xDA) {  // GenericPacket
            // Processar pacote genérico
            // ...
        }
    }
}
```

---

## 🖥️ Processamento no Backend (PHP)

### Arquivo: `ingest_aguada_ultra01.php`

```php
<?php
// Receber JSON do gateway
$json = file_get_contents('php://input');
$data = json_decode($json, true);

if ($data['packet_type'] !== 'aguadaUltrasonic01') {
    http_response_code(400);
    die('Invalid packet type');
}

// 1. Buscar configuração do nó
$node_id = intval($data['node_id']);
$cfg = get_node_config($node_id);  // Query no node_configs
if (!$cfg) {
    http_response_code(404);
    die("Node $node_id not configured");
}

// 2. Calcular dados derivados
$distance_cm = intval($data['distance_cm']) + $cfg['distance_offset_cm'];
$level_cm = $cfg['level_max_cm'] + $cfg['sensor_offset_cm'] - $distance_cm;
$level_cm = max(0, min($level_cm, $cfg['level_max_cm']));  // Clamp

$percentual = ($level_cm * 100) / $cfg['level_max_cm'];
$volume_l = ($level_cm * $cfg['vol_max_l']) / $cfg['level_max_cm'];

// 3. Detectar anomalias
$last_reading = get_last_reading($node_id);
$alert_type = 0;
if ($last_reading) {
    $delta = $level_cm - $last_reading['level_cm'];
    if (abs($delta) >= $cfg['rapid_change_threshold_cm']) {
        $alert_type = ($delta < 0) ? 1 : 2;  // 1=drop, 2=rise
    }
    // TODO: Check sensor_stuck (no change for X minutes)
}

// 4. Gravar dados processados
$stmt = $pdo->prepare("
    INSERT INTO telemetry_processed 
    (node_id, distance_cm, level_cm, percentual, volume_l, 
     rssi, ts_ms, flags, alert_type, location)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
");
$stmt->execute([
    $node_id,
    $distance_cm,
    $level_cm,
    round($percentual),
    round($volume_l),
    intval($data['rssi']),
    $data['ts_ms'],
    intval($data['flags']),
    $alert_type,
    $cfg['location']
]);

// 5. Responder ao gateway
http_response_code(200);
echo json_encode([
    'status' => 'ok',
    'node_id' => $node_id,
    'level_cm' => $level_cm,
    'alert_type' => $alert_type
]);
?>
```

---

## 📊 Tabela de Dados Processados

### Schema SQL

```sql
CREATE TABLE telemetry_processed (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    node_id TINYINT UNSIGNED NOT NULL,
    
    -- Raw data (from node)
    distance_cm SMALLINT NOT NULL,
    
    -- Processed data (server-calculated)
    level_cm SMALLINT NOT NULL,
    percentual TINYINT UNSIGNED NOT NULL,
    volume_l INT UNSIGNED NOT NULL,
    
    -- Metadata (from gateway)
    rssi TINYINT NOT NULL,
    ts_ms BIGINT UNSIGNED NOT NULL,
    
    -- Status
    flags TINYINT UNSIGNED NOT NULL DEFAULT 0,
    alert_type TINYINT UNSIGNED NOT NULL DEFAULT 0,
    location VARCHAR(32) NOT NULL,
    
    -- Timestamp
    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_node_ts (node_id, ts_ms),
    INDEX idx_alerts (alert_type, received_at),
    INDEX idx_location (location)
) ENGINE=InnoDB;
```

---

## 🔧 Configuração de Novo Nó

### 1. Determinar node_id
```bash
# No node firmware, definir:
#define NODE_ID 42  // Único em toda rede
```

### 2. Extrair MAC Address
```bash
cd ~/firmware_aguada/firmware/node_ultra1
idf.py -p /dev/ttyUSB0 monitor --no-reset

# Procurar linha:
# I (1234) node_ultra1: Node MAC: C8:2B:96:AA:BB:CC
```

### 3. Cadastrar no Backend
```sql
INSERT INTO node_configs 
(node_id, mac, location, sensor_offset_cm, level_max_cm, vol_max_l)
VALUES 
(42, 'C8:2B:96:AA:BB:CC', 'Tank D', 20, 450, 80000);
```

### 4. Flash e Testar
```bash
idf.py flash monitor
```

---

## 📈 Comparação de Pacotes

| Característica | aguadaUltrasonic01 | SensorPacketV1 | GenericPacket |
|----------------|-------------------|----------------|---------------|
| **Tamanho** | 13 bytes | 28 bytes | 20-250 bytes |
| **Dados transmitidos** | distance_cm | 6 campos | N pares chave-valor |
| **Processamento node** | Mínimo | Médio | Variável |
| **Processamento server** | Máximo | Mínimo | Médio |
| **Calibração** | Remota (BD) | Requer reflash | Remota (BD) |
| **Latência** | +50ms (server) | Imediato | +20ms (parsing) |
| **Escalabilidade** | Excelente (100+) | Boa (50+) | Boa (50+) |
| **Offline** | ❌ Não funciona | ✅ Funciona | ⚠️ Parsing local |
| **Uso bateria** | Ótimo | Bom | Médio |

---

## 🚀 Migração de SensorPacketV1 para aguadaUltrasonic01

### Passo 1: Atualizar Backend

```php
// Criar tabela node_configs (ver schema acima)
// Migrar configurações existentes:

INSERT INTO node_configs (node_id, mac, location, sensor_offset_cm, level_max_cm, vol_max_l)
SELECT 
    DISTINCT node_id, 
    CONCAT(HEX(mac[0]), ':', HEX(mac[1]), ':', ...) AS mac,
    'Unknown' AS location,
    20 AS sensor_offset_cm,  -- Default
    450 AS level_max_cm,
    80000 AS vol_max_l
FROM telemetry_raw
WHERE node_id IS NOT NULL;

-- Atualizar manualmente o campo 'location' para cada nó
UPDATE node_configs SET location = 'Reservoir A' WHERE node_id = 1;
```

### Passo 2: Modificar Gateway

```c
// gateway_devkit_v1/main/main.c
// Adicionar suporte para ambos os pacotes (backward compatibility)

void espnow_recv_cb(...) {
    uint8_t magic = data[0];
    
    if (magic == AGUADA_ULTRA01_MAGIC) {
        // Novo protocolo aguadaUltrasonic01
        handle_aguada_ultra01(data, len, info);
    }
    else if (data[0] == 0x01) {  // version=1 (SensorPacketV1)
        // Protocolo legado SensorPacketV1
        handle_sensor_packet_v1(data, len, info);
    }
}
```

### Passo 3: Reflash Nodes Gradualmente

```bash
# Começar com 1 nó teste
cd ~/firmware_aguada/firmware/node_ultra1
# Modificar código para usar aguadaUltrasonic01
idf.py flash monitor

# Verificar dashboard por 24h
# Se OK, migrar demais nós
```

---

## ⚠️ Limitações e Considerações

### Dependência de Backend
- ❌ **Não funciona offline**: Todos os cálculos dependem do servidor
- ⚠️ **Latência adicional**: +20-50ms para processar no servidor
- ⚠️ **Single point of failure**: Se backend cai, dados ficam "crus"

**Mitigação**: Gateway pode implementar cache de `node_configs` e processar localmente em caso de backend offline.

### Calibração
- ✅ **Facilita ajustes**: Alterar offsets sem reflash
- ⚠️ **Requer BD atualizado**: Configuração inconsistente = dados errados

**Mitigação**: API de validação antes de salvar configs.

### Escalabilidade
- ✅ **Eficiente para 100+ nós**: Menos dados na rede
- ⚠️ **Carga no servidor**: Processamento síncrono pode gargalar

**Mitigação**: Usar fila assíncrona (RabbitMQ, Redis) para processar telemetria.

---

## 🎓 Casos de Uso Recomendados

### ✅ USE aguadaUltrasonic01 quando:
- Rede com **50+ sensores**
- **Calibração frequente** (semanal/mensal)
- **Instalação temporária** (protótipos, testes)
- **Bateria crítica** (toda economia importa)
- **Geometria complexa** (cálculos no servidor são vantajosos)

### ❌ NÃO USE aguadaUltrasonic01 quando:
- **Sistema crítico offline** (bomba de emergência)
- **Latência < 1 segundo** (decisões em tempo real no edge)
- **Servidor instável** (backend pode cair frequentemente)
- **Sensores heterogêneos** (use GenericPacket)

---

## 📚 Referências

- ESP-NOW Protocol: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
- Packed Structures: https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html
- Binary Protocols Best Practices: https://developers.google.com/protocol-buffers

---

## 🔄 Changelog

### v1.0 (17/12/2025)
- Primeira versão do protocolo aguadaUltrasonic01
- Estrutura de 13 bytes
- Suporte para node_id (0-255)
- Flags para low_battery e sensor_error
- Documentação completa de integração server-side
