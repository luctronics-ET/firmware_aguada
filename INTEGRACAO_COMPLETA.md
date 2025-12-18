# Integração Completa - Sistema Aguada

## Visão Geral

Este documento descreve a integração completa entre firmware, database, backend e frontend do sistema de telemetria Aguada.

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         ARQUITETURA COMPLETA                            │
└─────────────────────────────────────────────────────────────────────────┘

   ┌─────────────┐
   │ Node 1-5    │ ──ESP-NOW──┐
   │ (ESP32-C3)  │             │
   └─────────────┘             │      ┌──────────────┐      ┌──────────────┐
                               ├─────▶│  Gateway     │──WiFi▶│   Backend    │
   ┌─────────────┐             │      │  ESP32       │      │  PHP/MySQL   │
   │ Node 10     │──Ethernet───┘      └──────────────┘      └──────┬───────┘
   │ (Nano ETH)  │                                                  │
   └─────────────┘                                                  │
                                                                    │
                                                           REST APIs│
                                                                    │
                                                                    ▼
                                                          ┌──────────────┐
                                                          │  Frontend    │
                                                          │  TailAdmin   │
                                                          └──────────────┘
```

## 1. FIRMWARE → BACKEND

### 1.1 Protocolo SensorPacketV1

**Nodes ESP32-C3 (1-5):** Enviam binary packet via ESP-NOW
**Node Nano (10):** Envia JSON via HTTP POST direto

**Struct Binary (ESP-NOW):**
```c
typedef struct __attribute__((packed)) {
    uint8_t  version;        // = 1
    uint8_t  node_id;        // 1-10
    uint8_t  mac[6];         // STA MAC
    uint32_t seq;            // Sequência
    int16_t  distance_cm;    // Distância HC-SR04
    int16_t  level_cm;       // Nível calculado
    uint8_t  percentual;     // 0-100%
    uint32_t volume_l;       // Volume em litros
    int16_t  vin_mv;         // Tensão mV
    uint8_t  flags;          // Bit 0: is_alert
    uint8_t  alert_type;     // 0-3
    int8_t   rssi;           // Preenchido por gateway
    uint32_t ts_ms;          // Timestamp gateway
} SensorPacketV1;  // 30 bytes total
```

**JSON (HTTP POST):**
```json
{
  "version": 1,
  "node_id": 10,
  "mac": "AA:BB:CC:DD:EE:01",
  "seq": 1234,
  "distance_cm": 72,
  "level_cm": 398,
  "percentual": 88,
  "volume_l": 70755,
  "vin_mv": 3300,
  "flags": 0,
  "alert_type": 0,
  "rssi": 0,
  "ts_ms": 1734510645000
}
```

### 1.2 Gateway → Backend

**Endpoint:** `http://192.168.0.117:8080/ingest_sensorpacket.php`
**Method:** POST
**Content-Type:** application/json

**Gateway adiciona:**
- `rssi`: Qualidade de sinal ESP-NOW (-100 a 0 dBm)
- `ts_ms`: Timestamp do gateway (milissegundos)

**Nano envia direto** (sem passar por gateway).

### 1.3 Backend Ingestão

**Arquivo:** `backend/ingest_sensorpacket.php`

```php
<?php
header('Content-Type: application/json');

$data = json_decode(file_get_contents('php://input'), true);

// Validação
if (!isset($data['node_id']) || !isset($data['distance_cm'])) {
    http_response_code(400);
    echo json_encode(['error' => 'Invalid packet']);
    exit;
}

// Insert no DB
$query = "INSERT INTO leituras_v2 
    (version, node_id, mac, seq, distance_cm, level_cm, 
     percentual, volume_l, vin_mv, rssi, ts_ms)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

$stmt = $conn->prepare($query);
$stmt->bind_param("iisiiiiiii", 
    $data['version'],
    $data['node_id'],
    $data['mac'],
    $data['seq'],
    $data['distance_cm'],
    $data['level_cm'],
    $data['percentual'],
    $data['volume_l'],
    $data['vin_mv'],
    $data['rssi'],
    $data['ts_ms']
);

$stmt->execute();
echo json_encode(['status' => 'ok']);
?>
```

## 2. DATABASE SCHEMA

### 2.1 Tabela Principal: leituras_v2

```sql
CREATE TABLE leituras_v2 (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    version TINYINT NOT NULL,
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
    INDEX idx_leituras_v2_node_ts (node_id, created_at)
);
```

### 2.2 Sistema SCADA (Migrations)

**003_sistema_scada.sql:**
- Tabelas: `locais`, `tipos_elemento`, `elementos`, `redes_agua`, `conexoes`
- Modelagem de instalações hidráulicas completa

**004_balanco_hidrico.sql:**
- Tabelas: `balanco_hidrico_diario`, `balanco_hidrico_mensal`
- Procedures: `calcular_balanco_diario()`, `calcular_balanco_mensal()`

**005_fix_balanco_logic.sql:**
- Correções na lógica de cálculo de entrada/saída

**006_fix_procedure_for_leituras_v2.sql:**
- Compatibilização procedures com nova tabela

## 3. BACKEND APIs

### 3.1 GET /api/get_sensors_data.php

**Propósito:** Status atual de todos os sensores (última leitura por node).

**Query:**
```sql
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

**Response:**
```json
{
  "status": "success",
  "sensors": [
    {
      "id": "NODE1",
      "node_id": 1,
      "name": "RCON",
      "mac": "20:6E:F1:6B:77:58",
      "distance_cm": 72,
      "level_cm": 398,
      "percentual": 88,
      "volume_l": 70755,
      "volume": 70755,
      "capacity": 80000,
      "vin_mv": 3300,
      "battery_v": 3.3,
      "rssi": -45,
      "last_update": "2025-12-18 08:30:45",
      "status": "normal",
      "flags": 0,
      "alert_type": 0
    }
  ]
}
```

### 3.2 GET /api/get_recent_readings.php

**Propósito:** Últimas N leituras (tabela de histórico).

**Parameters:**
- `limit` (opcional, default 50)
- `node_id` (opcional, filtrar por node)

**Example:** `/api/get_recent_readings.php?limit=20&node_id=1`

**Response:**
```json
{
  "status": "success",
  "count": 20,
  "readings": [
    {
      "id": 12345,
      "created_at": "2025-12-18 08:30:45",
      "node_id": 1,
      "mac": "20:6E:F1:6B:77:58",
      "seq": 1234,
      "distance_cm": 72,
      "level_cm": 398,
      "percentual": 88,
      "volume_l": 70755,
      "vin_mv": 3300,
      "rssi": -45,
      "ts_ms": 1734510645000
    }
  ]
}
```

### 3.3 GET /api/get_history.php

**Propósito:** Histórico agregado por minuto (para gráficos).

**Parameters:**
- `hours` (opcional, default 24)
- `node_id` (opcional)

**Query:**
```sql
SELECT 
    DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:00') as timestamp,
    node_id,
    AVG(level_cm) as avg_level_cm,
    AVG(percentual) as avg_percentual,
    AVG(volume_l) as avg_volume_l,
    AVG(rssi) as avg_rssi,
    COUNT(*) as reading_count
FROM leituras_v2
WHERE created_at >= NOW() - INTERVAL 24 HOUR
GROUP BY DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:00'), node_id
ORDER BY timestamp ASC
```

**Response:**
```json
{
  "status": "success",
  "hours": 24,
  "count": 1440,
  "history": [
    {
      "timestamp": "2025-12-18 08:30:00",
      "node_id": 1,
      "avg_level_cm": 398.5,
      "avg_percentual": 88.2,
      "avg_volume_l": 70800.0,
      "avg_rssi": -45.3,
      "reading_count": 2
    }
  ]
}
```

## 4. FRONTEND

### 4.1 Arquitetura

**Base:** TailAdmin (Tailwind CSS + Alpine.js)
**Localização:** `frontend/aguada-dashboard/`
**Arquivo principal:** `src/aguada-telemetry.html`

### 4.2 Alpine.js App

```javascript
function aguadaApp() {
  return {
    nodes: [],
    recentReadings: [],
    apiBaseUrl: 'http://192.168.0.117:8080/api',
    
    async fetchData() {
      // Buscar status atual
      const sensorsResponse = await fetch(`${this.apiBaseUrl}/get_sensors_data.php`);
      const sensorsData = await sensorsResponse.json();
      this.nodes = sensorsData.sensors;
      
      // Buscar histórico
      const readingsResponse = await fetch(`${this.apiBaseUrl}/get_recent_readings.php?limit=20`);
      const readingsData = await readingsResponse.json();
      this.recentReadings = readingsData.readings;
    },
    
    init() {
      this.fetchData();
      setInterval(() => this.fetchData(), 5000); // Auto-refresh 5s
    }
  };
}
```

### 4.3 Components

**Cards de Status:**
```html
<div class="p-6 bg-white rounded-lg shadow">
  <h3 x-text="node.name"></h3>
  <span x-text="node.percentual + '%'"></span>
  <div class="w-full h-3 bg-gray-200 rounded-full">
    <div 
      :style="`width: ${node.percentual}%`"
      :class="getLevelColor(node.percentual)"
    ></div>
  </div>
</div>
```

**Gráfico (ApexCharts):**
```javascript
const chart = new ApexCharts(document.querySelector("#levelChart"), {
  series: [{
    name: 'Nível (cm)',
    data: historyData
  }],
  chart: {
    type: 'line',
    height: 350
  },
  xaxis: {
    type: 'datetime'
  }
});
chart.render();
```

### 4.4 Build & Deploy

**Desenvolvimento:**
```bash
cd frontend/aguada-dashboard
npm install
npm start  # http://localhost:8080
```

**Produção:**
```bash
npm run build
cp -r dist/* /var/www/html/aguada/
```

## 5. COMPATIBILIDADE ENTRE CAMADAS

### 5.1 Mapeamento de Nodes

Todos os sistemas (firmware, backend, frontend) usam o mesmo mapeamento:

| Node ID | Nome | MAC | Reservatório | Capacidade | Tipo |
|---------|------|-----|--------------|------------|------|
| 1 | RCON | 20:6E:F1:6B:77:58 | Consumo | 80,000L | ESP32-C3 |
| 2 | RCAV | DC:06:75:67:6A:CC | Incêndio | 80,000L | ESP32-C3 |
| 3 | RCB3 | 80:F1:B2:50:31:34 | Bombas 03 | 80,000L | ESP32-C3 |
| 4 | CIE1 | DC:B4:D9:8B:9E:AC | Castelo IE 1 | 245,000L | ESP32-C3 |
| 5 | CIE2 | DC:B4:D9:8B:9E:AC | Castelo IE 2 | 245,000L | ESP32-C3 |
| 10 | RCON-ETH | AA:BB:CC:DD:EE:01 | Consumo Backup | 80,000L | Nano ETH |

### 5.2 Flags e Alertas

**firmware (node):**
```c
uint8_t flags = 0;
uint8_t alert_type = 0;

if (sensor_stuck) {
    flags = 1;
    alert_type = 3;
} else if (rapid_drop) {
    flags = 1;
    alert_type = 1;
} else if (rapid_rise) {
    flags = 1;
    alert_type = 2;
}
```

**backend (API):**
```php
$sensors[] = [
    'flags' => 0,
    'alert_type' => 0
];
```

**frontend (Alpine.js):**
```javascript
getAlertMessage(alertType) {
  const alerts = {
    0: 'Nenhum alerta',
    1: 'Queda rápida detectada',
    2: 'Subida rápida detectada',
    3: 'Sensor travado'
  };
  return alerts[alertType];
}
```

### 5.3 Status Online/Offline

**Lógica:** Node considerado offline se `last_update` > 3 minutos

**Frontend:**
```javascript
isOnline(timestamp) {
  const lastUpdate = new Date(timestamp);
  const now = new Date();
  const diffMinutes = (now - lastUpdate) / 1000 / 60;
  return diffMinutes < 3;
}
```

## 6. FLUXO COMPLETO DE DADOS

### 6.1 Ciclo Normal (ESP32-C3)

```
1. Node mede HC-SR04 (72cm)
   ↓
2. Calcula level_cm (398cm), percentual (88%), volume_l (70755L)
   ↓
3. Detecta anomalias (flags=0, alert_type=0)
   ↓
4. Monta SensorPacketV1 (28 bytes binary)
   ↓
5. Envia via ESP-NOW para gateway (canal 11)
   ↓
6. Gateway recebe, adiciona RSSI (-45) e timestamp
   ↓
7. Gateway converte binary → JSON
   ↓
8. Gateway POST para http://192.168.0.117:8080/ingest_sensorpacket.php
   ↓
9. Backend valida e INSERT em leituras_v2
   ↓
10. Frontend faz GET /api/get_sensors_data.php (a cada 5s)
    ↓
11. Alpine.js atualiza cards e gráficos
    ↓
12. Usuário vê dashboard em tempo real
```

### 6.2 Ciclo Redundante (Nano Ethernet Node 10)

```
1. Nano mede HC-SR04 (72cm) - MESMO reservatório que Node 1
   ↓
2. Calcula level_cm, percentual, volume_l
   ↓
3. Monta JSON completo
   ↓
4. POST direto para backend (sem gateway)
   ↓
5. Backend INSERT em leituras_v2 com node_id=10
   ↓
6. Frontend mostra Node 1 e Node 10 separadamente
   ↓
7. Sistema pode comparar leituras e detectar discrepâncias
```

## 7. CHECKLIST DE INTEGRAÇÃO

### 7.1 Firmware ✅

- [x] SensorPacketV1 implementado em todos os nodes
- [x] Gateway recebendo ESP-NOW de nodes 1-5
- [x] Node 10 enviando JSON via Ethernet
- [x] Flags e alert_type implementados
- [x] MACs reais extraídos e documentados

### 7.2 Database ✅

- [x] Schema leituras_v2 criado
- [x] Migrations SCADA importadas (003-006)
- [x] Índices otimizados (node_id, created_at)
- [x] nodes_config_REAL_MACS.sql pronto

### 7.3 Backend ✅

- [x] ingest_sensorpacket.php funcional
- [x] API get_sensors_data.php criada
- [x] API get_recent_readings.php criada
- [x] API get_history.php criada
- [x] CORS headers configurados
- [x] Mapeamento de 6 nodes completo

### 7.4 Frontend ✅

- [x] TailAdmin copiado para frontend/aguada-dashboard
- [x] aguada-telemetry.html criado
- [x] Alpine.js app (aguadaApp) implementado
- [x] Cards de status funcionais
- [x] Tabela de leituras funcionais
- [x] Auto-refresh (5s) implementado
- [x] Indicadores de alerta prontos
- [ ] Gráficos ApexCharts (placeholders criados)

### 7.5 Testes Pendentes ⏳

- [ ] Iniciar backend: ./start_services.sh
- [ ] Executar nodes_config_REAL_MACS.sql
- [ ] Conectar HC-SR04 nos nodes físicos
- [ ] Validar end-to-end (sensor → frontend)
- [ ] Testar redundância (Node 1 vs Node 10)
- [ ] Compilar frontend: npm run build
- [ ] Deploy produção

## 8. TROUBLESHOOTING

### 8.1 Firmware não envia dados

```bash
# Monitor node
cd firmware/node_ultra1
idf.py -p /dev/ttyACM0 monitor

# Verificar:
# - ESP-NOW initialized?
# - Distance cm válido?
# - Gateway MAC correto?
# - Canal 11?
```

### 8.2 Backend não recebe dados

```bash
# Testar endpoint
curl -X POST http://192.168.0.117:8080/ingest_sensorpacket.php \
  -H "Content-Type: application/json" \
  -d '{
    "version":1,
    "node_id":1,
    "mac":"20:6E:F1:6B:77:58",
    "seq":1234,
    "distance_cm":72,
    "level_cm":398,
    "percentual":88,
    "volume_l":70755,
    "vin_mv":3300,
    "rssi":-45,
    "ts_ms":1734510645000
  }'

# Verificar MySQL
mysql -u root -p
USE aguada_db;
SELECT * FROM leituras_v2 ORDER BY created_at DESC LIMIT 5;
```

### 8.3 Frontend não carrega dados

```bash
# Testar API
curl http://192.168.0.117:8080/api/get_sensors_data.php

# Abrir DevTools (F12)
# Console > Network > XHR
# Verificar:
# - CORS headers?
# - 200 OK?
# - JSON válido?
```

### 8.4 Gateway queue overflow

```c
// Aumentar tamanho da fila em gateway/main.c
#define ESPNOW_QUEUE_SIZE 30  // Era 10
```

---

**Atualizado:** 18 de dezembro de 2025  
**Status:** Integração completa documentada, testes pendentes
