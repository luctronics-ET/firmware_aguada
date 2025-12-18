# Sistema AGUADA - Telemetria de Água CMASM
## Regras de Negócio e Especificações Técnicas

**Data**: 16/12/2025  
**Versão**: 2.0  
**Status**: Produção (2 sensores ativos)

---

## 1. DESCRIÇÃO RESUMIDA DO SISTEMA

### 1.1 Visão Geral

Sistema de telemetria e monitoramento hídrico baseado em ESP32 para o Centro de Mísseis e Armas Submarinas da Marinha (CMASM), localizado na Ilha do Engenho, São Gonçalo - RJ.

**Objetivo**: Monitorar em tempo real o nível de água em reservatórios e cisternas da infraestrutura predial do CMASM, permitindo:
- Controle operacional centralizado
- Detecção automática de vazamentos
- Balanço hídrico diário
- Alertas de nível crítico
- Histórico de consumo e abastecimento

### 1.2 Arquitetura

```
┌─────────────────┐
│  ESP32-C3 Node  │ ──┐
│  (Ultrassônico) │   │
└─────────────────┘   │
                      │ ESP-NOW (2.4 GHz)
┌─────────────────┐   │ Channel 11
│  ESP32-C3 Node  │ ──┤
│  (Ultrassônico) │   │
└─────────────────┘   │
                      ↓
                ┌──────────────┐
                │ ESP32 Gateway│
                │  (DevKit V1) │
                └──────┬───────┘
                       │ HTTP POST / USB Serial
                       ↓
                ┌──────────────┐
                │ PHP Backend  │
                │ MySQL DB     │
                └──────┬───────┘
                       │ REST API
                       ↓
                ┌──────────────┐
                │ Web Dashboard│
                │ SCADA / Maps │
                └──────────────┘
```

### 1.3 Componentes

**Hardware em Produção:**
- **2x ESP32-C3 Supermini**: Sensores ultrassônicos HC-SR04 + ADC bateria
  - NODE-01 (CON): Castelo de Consumo - 80m³
  - NODE-02 (CAV): Castelo de Incêndio - 80m³
- **1x ESP32 DevKit V1**: Gateway receptor ESP-NOW → HTTP

**Hardware Planejado:**
- **3x ESP32-C3**: 
  - NODE-03 (CB3): Casa de Bombas - 80m³
  - NODE-04a (CIE1): Cisterna IE 01 - 245m³
  - NODE-04b (CIE2): Cisterna IE 02 - 245m³

**Software:**
- **Firmware**: ESP-IDF (C/C++)
- **Backend**: PHP 8.x + MySQL 8.x
- **Frontend**: HTML/CSS/JavaScript (Tailwind CSS, Leaflet.js)

### 1.4 Rede Hídrica CMASM

O sistema monitora 4 redes hídricas distintas:

| Rede | Tipo | Função | Qualidade |
|------|------|--------|-----------|
| **Abastecimento** | Captação | Cisternas CIE1/CIE2 ← Fonte externa | Não tratada |
| **Consumo** | Distribuição | Reservatório CON → Áreas Azul/Vermelha | Potável |
| **Incêndio** | Emergência | Reservatório CAV → Hidrantes | Não tratada |
| **Esgoto** | Coleta | Instalações → Tratamento | N/A |

**Fluxo de Água:**
```
Cisternas CIE1/CIE2 (245m³ cada)
    ↓ VB03-IN1
Casa de Bombas B03 (80m³)
    ├─ VB03-OUT1 → Castelo Incêndio CAV (80m³) → Rede incêndio
    └─ VB03-OUT2 → Castelo Consumo CON (80m³) → Rede consumo
```

---

## 2. PACOTE DE DADOS (TELEMETRY PACKET V1)

### 2.1 Estrutura Binária (`SensorPacketV1`)

**Arquivo**: `firmware/common/telemetry_packet.h`  
**Protocolo**: ESP-NOW (binary payload, packed struct)  
**Versão**: 1  
**Tamanho**: 28 bytes (packed, sem padding)

```cpp
typedef struct __attribute__((packed)) {
    // ========== CABEÇALHO (9 bytes) ==========
    uint8_t  version;        // Versão do protocolo (sempre = 1)
    uint8_t  node_id;        // ID do sensor (1-255)
    uint8_t  mac[6];         // MAC address do ESP32 transmissor
    uint32_t seq;            // Sequência monotônica (contador de pacotes)

    // ========== MEDIÇÕES DO SENSOR (11 bytes) ==========
    int16_t  distance_cm;    // Distância medida pelo ultrassônico (cm)
    int16_t  level_cm;       // Nível de água calculado (cm)
    uint8_t  percentual;     // Nível percentual (0-100%)
    uint32_t volume_l;       // Volume calculado (litros)
    int16_t  vin_mv;         // Tensão de alimentação (milivolts)

    // ========== METADADOS DO GATEWAY (5 bytes) ==========
    int8_t   rssi;           // RSSI do sinal ESP-NOW (dBm)
    uint32_t ts_ms;          // Timestamp do gateway (milissegundos)
} SensorPacketV1;

#define SENSOR_PACKET_VERSION 1
```

### 2.2 Campos Detalhados

#### 2.2.1 Cabeçalho (preenchido pelo sensor)

| Campo | Tipo | Bytes | Descrição | Exemplo |
|-------|------|-------|-----------|---------|
| `version` | uint8 | 1 | Versão do protocolo | `1` |
| `node_id` | uint8 | 1 | ID único do sensor | `1` (CON), `2` (CAV) |
| `mac[6]` | uint8[6] | 6 | MAC address do ESP32 | `{0x34,0x85,0x18,0xAB,0xCD,0xEF}` |
| `seq` | uint32 | 4 | Contador de pacotes (incrementa a cada envio) | `1234` |

#### 2.2.2 Medições (preenchido pelo sensor)

| Campo | Tipo | Bytes | Descrição | Range | Unidade |
|-------|------|-------|-----------|-------|---------|
| `distance_cm` | int16 | 2 | Distância sensor → superfície água | 0-500 | cm |
| `level_cm` | int16 | 2 | Nível de água no tanque | 0-450 | cm |
| `percentual` | uint8 | 1 | Nível percentual | 0-100 | % |
| `volume_l` | uint32 | 4 | Volume de água | 0-245000 | litros |
| `vin_mv` | int16 | 2 | Tensão de alimentação | 0-5000 | mV |

#### 2.2.3 Metadados do Gateway (preenchido pelo gateway)

| Campo | Tipo | Bytes | Descrição | Range | Unidade |
|-------|------|-------|-----------|-------|---------|
| `rssi` | int8 | 1 | Intensidade do sinal | -100 a 0 | dBm |
| `ts_ms` | uint32 | 4 | Timestamp Unix (milissegundos) | 0-2³² | ms |

### 2.3 Cálculos no Sensor (Node)

**Arquivo**: `firmware/components/level_calculator/level_calculator.h`

#### Parâmetros do Reservatório

```cpp
struct Model {
    int level_max_cm;       // Altura máxima de água (ex: 450 cm)
    int sensor_offset_cm;   // Distância sensor → topo do tanque (ex: 20 cm)
    int vol_max_l;          // Capacidade máxima (ex: 80000 L)
};
```

**Configuração NODE-01 (CON):**
```cpp
Model con_model = {
    .level_max_cm = 450,      // 4.5 metros de altura útil
    .sensor_offset_cm = 20,   // Sensor 20cm acima do topo
    .vol_max_l = 80000        // 80 m³ = 80.000 litros
};
```

**Configuração NODE-02 (CAV):**
```cpp
Model cav_model = {
    .level_max_cm = 450,
    .sensor_offset_cm = 20,
    .vol_max_l = 80000        // Mesma capacidade que CON
};
```

**Configuração NODE-04 (CIE1/CIE2) - planejado:**
```cpp
Model cie_model = {
    .level_max_cm = 400,      // Cisternas mais baixas
    .sensor_offset_cm = 20,
    .vol_max_l = 245000       // 245 m³ = 245.000 litros
};
```

#### Algoritmo de Cálculo

```cpp
// 1. Calcular nível de água
int base = level_max_cm + sensor_offset_cm;  // Ex: 450 + 20 = 470 cm
int level_cm = base - distance_cm;            // Ex: 470 - 150 = 320 cm

// 2. Limitar ao range válido
if (level_cm < 0) level_cm = 0;
if (level_cm > level_max_cm) level_cm = level_max_cm;

// 3. Calcular percentual (aritmética inteira)
int percentual = (level_cm * 100) / level_max_cm;  // Ex: (320 * 100) / 450 = 71%

// 4. Calcular volume (proporção linear)
int64_t volume_l = ((int64_t)level_cm * (int64_t)vol_max_l) / (int64_t)level_max_cm;
// Ex: (320 * 80000) / 450 = 56.888 litros
```

**Exemplo Prático:**

| Medição | Valor | Cálculo |
|---------|-------|---------|
| Distância medida | 150 cm | HC-SR04 retorna |
| Base do cálculo | 470 cm | 450 + 20 |
| Nível calculado | 320 cm | 470 - 150 |
| Percentual | 71% | (320 × 100) / 450 |
| Volume | 56.888 L | (320 × 80000) / 450 |

### 2.4 Pipeline de Transmissão

```
┌─────────────────────────────────────────────────────────────┐
│ ESP32-C3 NODE                                               │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 1. HC-SR04: Medir distância (3 amostras, usar mediana) │ │
│ │    → distance_cm = 150 cm                               │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 2. level_calculator::compute()                          │ │
│ │    → level_cm = 320, percentual = 71, volume = 56888   │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 3. ADC: Ler tensão de bateria                           │ │
│ │    → vin_mv = 4200 mV (bateria cheia)                   │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 4. Preencher SensorPacketV1                             │ │
│ │    version=1, node_id=1, mac={...}, seq=1234            │ │
│ │    distance_cm=150, level_cm=320, percentual=71         │ │
│ │    volume_l=56888, vin_mv=4200                          │ │
│ │    rssi=0, ts_ms=0  (preenchido pelo gateway)           │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 5. ESP-NOW: Transmitir 28 bytes binários               │ │
│ │    → Broadcast ou unicast para gateway_mac              │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                          ↓ ESP-NOW (2.4 GHz)
┌─────────────────────────────────────────────────────────────┐
│ ESP32 GATEWAY (DevKit V1)                                   │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 6. Callback ESP-NOW (ISR context)                       │ │
│ │    → Receber 28 bytes                                   │ │
│ │    → Adicionar rssi (ex: -75 dBm)                       │ │
│ │    → Adicionar ts_ms (millis())                         │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 7. Queue: xQueueSendFromISR()                           │ │
│ │    → Enviar pacote para fila FreeRTOS                   │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 8. Task HTTP: xQueueReceive()                           │ │
│ │    → Converter SensorPacketV1 para JSON                 │ │
│ │    → POST http://backend/ingest_sensorpacket.php        │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                          ↓ HTTP POST (JSON)
┌─────────────────────────────────────────────────────────────┐
│ PHP BACKEND                                                 │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 9. ingest_sensorpacket.php                              │ │
│ │    → Validar JSON                                       │ │
│ │    → INSERT INTO leituras_v2                            │ │
│ └─────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 10. MySQL: Gravar registro                              │ │
│ │     → id, node_id, distance_cm, level_cm, percentual,   │ │
│ │       volume_l, vin_mv, rssi, created_at                │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 2.5 Formato JSON (Backend)

**POST → `backend/ingest_sensorpacket.php`:**

```json
{
  "version": 1,
  "node_id": 1,
  "mac": "34:85:18:AB:CD:EF",
  "seq": 1234,
  "distance_cm": 150,
  "level_cm": 320,
  "percentual": 71,
  "volume_l": 56888,
  "vin_mv": 4200,
  "rssi": -75,
  "ts_ms": 1702684800000
}
```

**GET ← `backend/api/get_sensors_data.php`:**

```json
{
  "sensors": [
    {
      "id": "NODE1",
      "node_id": 1,
      "name": "CON",
      "level": 71,
      "volume": 56888,
      "capacity": 80000,
      "distance_cm": 150,
      "level_cm": 320,
      "battery": 4200,
      "battery_v": 4.2,
      "rssi": -75,
      "status": "normal",
      "lastUpdate": "2025-12-16 14:30:15",
      "valve_in": true,
      "valve_out": true,
      "flow": true
    }
  ],
  "timestamp": "2025-12-16T14:30:15-03:00"
}
```

---

## 3. CÁLCULOS, TASKS E PROCESSAMENTOS

### 3.1 Processamento no Sensor (Node)

**Frequência**: A cada 30 segundos (configurable via `SLEEP_SECONDS`)

#### Task Principal: `app_main()`

```cpp
while (1) {
    // 1. Medir distância (3 tentativas, usar mediana)
    int distances[3];
    for (int i = 0; i < 3; i++) {
        distances[i] = ultrasonic01::read_cm(TRIG_GPIO, ECHO_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    int distance_cm = median(distances, 3);
    
    // 2. Calcular nível e volume
    level_calculator::Model model = {450, 20, 80000};
    auto result = level_calculator::compute(distance_cm, model);
    
    // 3. Ler bateria (ADC channel 0, escala 0-4095 mV)
    int vin_mv = adc_read_voltage();
    
    // 4. Construir pacote
    SensorPacketV1 packet = {
        .version = 1,
        .node_id = NODE_ID,
        .mac = {my_mac[0], my_mac[1], ...},
        .seq = seq_counter++,
        .distance_cm = distance_cm,
        .level_cm = result.level_cm,
        .percentual = result.percentual,
        .volume_l = result.volume_l,
        .vin_mv = vin_mv,
        .rssi = 0,
        .ts_ms = 0
    };
    
    // 5. Enviar via ESP-NOW
    esp_now_send(gateway_mac, (uint8_t*)&packet, sizeof(packet));
    
    // 6. LED feedback (3 blinks rápidos)
    led_blink(3, 150);
    
    // 7. Sleep até próximo ciclo
    vTaskDelay(pdMS_TO_TICKS(30000));
}
```

**Carga de Processamento:**
- Medição ultrassônica: ~50ms × 3 = 150ms
- Cálculos inteiros: ~1ms
- Transmissão ESP-NOW: ~10ms
- **Total ativo**: ~161ms a cada 30s = **0.5% duty cycle**

**Consumo Estimado:**
- Ativo (161ms): ~80mA
- Sleep (29.839s): ~10mA (Wi-Fi off, deep sleep desabilitado)
- **Média**: ~10.5 mA → Bateria 3000mAh dura ~286 horas (~12 dias)

### 3.2 Processamento no Gateway

**Arquitetura**: Queue-based ISR → Task pipeline (evita bloqueio em callback)

#### 3.2.1 Callback ESP-NOW (ISR Context)

```c
void espnow_recv_cb(const esp_now_recv_info_t *info, 
                    const uint8_t *data, int len) {
    if (len != sizeof(SensorPacketV1)) return;
    
    SensorPacketV1 packet;
    memcpy(&packet, data, sizeof(packet));
    
    // Adicionar metadados do gateway
    packet.rssi = info->rx_ctrl->rssi;
    packet.ts_ms = esp_timer_get_time() / 1000;
    
    // Enviar para fila (non-blocking)
    xQueueSendFromISR(espnow_queue, &packet, NULL);
}
```

#### 3.2.2 Task HTTP POST

```c
void http_post_task(void *arg) {
    SensorPacketV1 packet;
    
    while (1) {
        // Aguardar pacote na fila
        if (xQueueReceive(espnow_queue, &packet, portMAX_DELAY)) {
            
            // Converter para JSON
            char json[512];
            snprintf(json, sizeof(json),
                "{\"version\":%d,\"node_id\":%d,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
                "\"seq\":%u,\"distance_cm\":%d,\"level_cm\":%d,\"percentual\":%d,"
                "\"volume_l\":%u,\"vin_mv\":%d,\"rssi\":%d,\"ts_ms\":%u}",
                packet.version, packet.node_id,
                packet.mac[0], packet.mac[1], packet.mac[2],
                packet.mac[3], packet.mac[4], packet.mac[5],
                packet.seq, packet.distance_cm, packet.level_cm,
                packet.percentual, packet.volume_l, packet.vin_mv,
                packet.rssi, packet.ts_ms
            );
            
            // POST HTTP (blocking - OK em task context)
            esp_http_client_config_t config = {
                .url = INGEST_URL,
                .method = HTTP_METHOD_POST,
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json, strlen(json));
            
            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "HTTP POST OK: node=%d, seq=%u", 
                         packet.node_id, packet.seq);
            } else {
                ESP_LOGE(TAG, "HTTP POST FAIL: %s", esp_err_to_name(err));
            }
            
            esp_http_client_cleanup(client);
        }
    }
}
```

**Capacidade:**
- Queue size: 10 pacotes (ajustar para 20-30 se necessário para 5 nodes)
- Throughput: ~1 POST/segundo (cada POST ~500ms)
- Latência: < 2s (do callback até DB insert)

### 3.3 Processamento no Backend (PHP/MySQL)

#### 3.3.1 Ingestão de Dados

**Endpoint**: `backend/ingest_sensorpacket.php`

```php
// 1. Receber JSON
$json = file_get_contents('php://input');
$data = json_decode($json, true);

// 2. Validar campos obrigatórios
if (!isset($data['node_id'], $data['level_cm'], $data['volume_l'])) {
    http_response_code(400);
    die(json_encode(['error' => 'Missing fields']));
}

// 3. Preparar SQL (prepared statement)
$stmt = $conn->prepare("
    INSERT INTO leituras_v2 
    (node_id, distance_cm, level_cm, percentual, volume_l, vin_mv, rssi) 
    VALUES (?, ?, ?, ?, ?, ?, ?)
");

$stmt->bind_param('iiiiiii',
    $data['node_id'],
    $data['distance_cm'],
    $data['level_cm'],
    $data['percentual'],
    $data['volume_l'],
    $data['vin_mv'],
    $data['rssi']
);

// 4. Executar insert
if ($stmt->execute()) {
    http_response_code(201);
    echo json_encode(['success' => true, 'id' => $conn->insert_id]);
} else {
    http_response_code(500);
    echo json_encode(['error' => $stmt->error]);
}
```

**Throughput**: ~500 INSERTs/segundo (MySQL otimizado, índices em `node_id`, `created_at`)

#### 3.3.2 API de Leitura

**Endpoint**: `backend/api/get_sensors_data.php`

```sql
-- Query otimizada: Última leitura de cada node (usa MAX(id) para desempate)
SELECT 
    node_id,
    distance_cm,
    level_cm,
    percentual,
    volume_l,
    vin_mv,
    rssi,
    created_at as last_update
FROM leituras_v2
WHERE id IN (
    SELECT MAX(id)
    FROM leituras_v2
    WHERE (node_id, created_at) IN (
        SELECT node_id, MAX(created_at)
        FROM leituras_v2
        GROUP BY node_id
    )
    GROUP BY node_id
)
ORDER BY node_id
```

**Performance**: ~10ms para 5 nodes (índice composto em `(node_id, created_at)`)

### 3.4 Cálculos de Métricas (Frontend)

#### 3.4.1 Status do Sensor

```javascript
function getSensorStatus(level_percent, battery_mv) {
    // Bateria
    if (battery_mv < 3300) return 'critical'; // < 3.3V
    if (battery_mv < 3600) return 'warning';  // < 3.6V
    
    // Nível
    if (level_percent < 20) return 'alert';   // < 20%
    if (level_percent < 40) return 'warning'; // < 40%
    
    return 'normal';
}
```

#### 3.4.2 Qualidade do Sinal

```javascript
function getSignalQuality(rssi_dbm) {
    if (rssi_dbm > -50) return { quality: 'excellent', bars: 5 };
    if (rssi_dbm > -60) return { quality: 'good', bars: 4 };
    if (rssi_dbm > -70) return { quality: 'fair', bars: 3 };
    if (rssi_dbm > -80) return { quality: 'poor', bars: 2 };
    return { quality: 'very_poor', bars: 1 };
}
```

### 3.5 Balanço Hídrico (Análise Diária)

**Objetivo**: Detectar vazamentos comparando consumo real vs. esperado

#### 3.5.1 Conceitos

```
BALANÇO = VOLUME_FINAL - VOLUME_INICIAL

• BALANÇO > 0 → ENTRADA (abastecimento do reservatório)
• BALANÇO < 0 → SAÍDA (consumo de água)
```

**Exemplo NODE-01 (CON):**
```
08:00 → Volume = 60.000 L
18:00 → Volume = 45.000 L

BALANÇO = 45.000 - 60.000 = -15.000 L
→ CONSUMO = 15.000 L em 10 horas = 1.500 L/h
```

#### 3.5.2 Detecção de Vazamento

```sql
-- View: Consumo diário por node
CREATE VIEW daily_consumption AS
SELECT 
    node_id,
    DATE(created_at) as dia,
    MIN(volume_l) as volume_min,
    MAX(volume_l) as volume_max,
    (MAX(volume_l) - MIN(volume_l)) as balanco,
    ABS(MIN(volume_l) - MAX(volume_l)) as consumo_abs,
    COUNT(*) as leituras
FROM leituras_v2
GROUP BY node_id, DATE(created_at);

-- Comparação com média histórica (7 dias)
SELECT 
    d.node_id,
    d.dia,
    d.consumo_abs as consumo_hoje,
    AVG(h.consumo_abs) as consumo_media_7d,
    (d.consumo_abs - AVG(h.consumo_abs)) as diferenca,
    ((d.consumo_abs - AVG(h.consumo_abs)) / AVG(h.consumo_abs) * 100) as variacao_percent,
    CASE
        WHEN d.consumo_abs > AVG(h.consumo_abs) * 1.5 THEN 'CRÍTICO'
        WHEN d.consumo_abs > AVG(h.consumo_abs) * 1.2 THEN 'ALERTA'
        ELSE 'NORMAL'
    END as status_vazamento
FROM daily_consumption d
JOIN daily_consumption h ON h.node_id = d.node_id 
    AND h.dia BETWEEN DATE_SUB(d.dia, INTERVAL 7 DAY) AND DATE_SUB(d.dia, INTERVAL 1 DAY)
WHERE d.dia = CURDATE()
GROUP BY d.node_id, d.dia;
```

**Níveis de Alerta:**
- **NORMAL**: Consumo < Média + 20%
- **ALERTA**: Consumo entre Média + 20% e Média + 50%
- **CRÍTICO**: Consumo > Média + 50%

**Exemplo Prático:**
```
NODE-01 (CON):
├─ Média 7 dias: 12.000 L/dia
├─ Consumo hoje: 18.000 L/dia
├─ Variação: +50% (6.000 L acima do esperado)
└─ STATUS: 🔴 CRÍTICO - Possível vazamento severo!

NODE-02 (CAV):
├─ Média 7 dias: 500 L/dia (uso esporádico incêndio)
├─ Consumo hoje: 650 L/dia
├─ Variação: +30%
└─ STATUS: 🟡 ALERTA - Consumo acima do normal
```

### 3.6 Tasks Agendadas (Cron / Scheduled)

#### 3.6.1 Limpeza de Dados Antigos

```sql
-- Rodar diariamente às 03:00
-- Manter 90 dias de histórico, deletar mais antigos
DELETE FROM leituras_v2 
WHERE created_at < DATE_SUB(NOW(), INTERVAL 90 DAY);
```

#### 3.6.2 Cálculo de Relatórios

```sql
-- Stored procedure: Calcular balanço do dia anterior
CALL calcular_balanco_diario(DATE_SUB(CURDATE(), INTERVAL 1 DAY));

-- Inserir em tabela de relatórios
INSERT INTO relatorios_diarios (node_id, dia, consumo, status)
SELECT node_id, dia, consumo_abs, status_vazamento
FROM view_vazamentos
WHERE dia = DATE_SUB(CURDATE(), INTERVAL 1 DAY);
```

#### 3.6.3 Notificações de Alerta

```php
// Script PHP: check_alerts.php (rodar a cada 5 minutos)
$alerts = $conn->query("
    SELECT node_id, percentual, battery_v 
    FROM (
        SELECT node_id, percentual, vin_mv/1000 as battery_v,
               ROW_NUMBER() OVER (PARTITION BY node_id ORDER BY created_at DESC) as rn
        FROM leituras_v2
        WHERE created_at > NOW() - INTERVAL 10 MINUTE
    ) t
    WHERE rn = 1
      AND (percentual < 20 OR battery_v < 3.3)
");

foreach ($alerts as $alert) {
    // Enviar email/SMS/Telegram
    sendAlert("NODE-{$alert['node_id']}: Nível {$alert['percentual']}%, Bateria {$alert['battery_v']}V");
}
```

---

## 4. RESUMO DE CONFIGURAÇÕES

### 4.1 Parâmetros por Node

| Node | ID | Local | Capacidade | Level Max | Offset | Frequência TX |
|------|----|-------|------------|-----------|--------|---------------|
| NODE-01 | 1 | CON | 80.000 L | 450 cm | 20 cm | 30s |
| NODE-02 | 2 | CAV | 80.000 L | 450 cm | 20 cm | 30s |
| NODE-03 | 3 | CB3 | 80.000 L | 450 cm | 20 cm | 30s (planejado) |
| NODE-04a | 4 | CIE1 | 245.000 L | 400 cm | 20 cm | 30s (planejado) |
| NODE-04b | 5 | CIE2 | 245.000 L | 400 cm | 20 cm | 30s (planejado) |

### 4.2 Thresholds e Alertas

| Métrica | Normal | Alerta | Crítico |
|---------|--------|--------|---------|
| **Nível** | ≥ 40% | 20-40% | < 20% |
| **Bateria** | ≥ 3.6V | 3.3-3.6V | < 3.3V |
| **RSSI** | > -70 dBm | -80 a -70 dBm | < -80 dBm |
| **Consumo** | Média ±20% | Média +20% a +50% | Média +50% |
| **Offline** | < 5 min | 5-15 min | > 15 min |

### 4.3 Capacidade do Sistema

**Atual (2 nodes):**
- Pacotes/hora: 2 nodes × 120 TX/hora = 240 pacotes/hora
- Registros/dia: 240 × 24 = 5.760 registros/dia
- Armazenamento: ~170 KB/dia (MySQL, comprimido ~50 KB)
- 90 dias: ~15 MB (~4.5 MB comprimido)

**Futuro (5 nodes):**
- Pacotes/hora: 5 nodes × 120 TX/hora = 600 pacotes/hora
- Registros/dia: 14.400 registros/dia
- Armazenamento: ~425 KB/dia (~125 KB comprimido)
- 90 dias: ~37 MB (~11 MB comprimido)

**Escalabilidade:**
- Gateway suporta até 20 nodes (limitação: queue size + HTTP throughput)
- Backend PHP/MySQL: ~10.000 INSERTs/min (MySQL otimizado)
- Frontend: Polling 10s OK para até 20 sensores

---

## 5. REFERÊNCIAS TÉCNICAS

### 5.1 Arquivos Principais

| Arquivo | Descrição |
|---------|-----------|
| `firmware/common/telemetry_packet.h` | Definição do protocolo binário |
| `firmware/components/level_calculator/level_calculator.h` | Algoritmo de cálculo de nível/volume |
| `firmware/components/ultrasonic01/ultrasonic01.h` | Driver HC-SR04 |
| `backend/ingest_sensorpacket.php` | Endpoint de ingestão |
| `backend/api/get_sensors_data.php` | API de leitura |
| `database/schema.sql` | Schema MySQL |
| `database/migrations/004_balanco_hidrico.sql` | Views e procedures de balanço |
| `docs/ARCHITECTURE.md` | Arquitetura completa do sistema |
| `docs/CORRECAO_BALANCO_HIDRICO.md` | Documentação de balanço hídrico |

### 5.2 Documentos Relacionados

- `README.md`: Visão geral do projeto
- `melhorias.md`: Roadmap de melhorias futuras
- `gateway_devkit_v1/ARCHITECTURE.md`: Arquitetura do gateway (queue pipeline)
- `backend/README.md`: Documentação do backend PHP
- `database/README.md`: Documentação do banco de dados

### 5.3 Coordenadas GPS (OpenStreetMap)

| Node | Reservatório | Latitude | Longitude | OSM Node ID |
|------|--------------|----------|-----------|-------------|
| NODE-01 | RCON (CON) | -22.8382494 | -43.1080894 | 13323767070 |
| NODE-02 | RCAV (CAV) | -22.8371134 | -43.1093961 | 13323767089 |
| NODE-03 | RB03 (CB3) | -22.8384951 | -43.1083615 | 13323767090 |
| NODE-04a | CIE1 | -22.8388258 | -43.1081429 | 13323767091 |
| NODE-04b | CIE2 | -22.8389166 | -43.1080755 | 13323767092 |

**Centro do mapa**: -22.8380, -43.1086 (Ilha do Engenho, São Gonçalo - RJ)

---

**Fim do Documento**
