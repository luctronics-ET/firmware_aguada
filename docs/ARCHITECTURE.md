# Aguada Telemetry - System Architecture

## Overview

Sistema distribuído de telemetria para monitoramento de nível de água usando ESP32 com protocolo ESP-NOW.

```
┌─────────────────────────────────────────────────────────────┐
│                    CAMPO (Field Deployment)                  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │  Node 1  │  │  Node 2  │  │  Node 3  │  │  Node 4  │    │
│  │ ESP32-C3 │  │ ESP32-C3 │  │ ESP32-C3 │  │ ESP32-C3 │    │
│  │ HC-SR04  │  │ HC-SR04  │  │ HC-SR04  │  │ HC-SR04  │    │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘    │
│       │             │             │             │            │
│       └─────────────┴──ESP-NOW────┴─────────────┘            │
│                     (Channel 11)                             │
│                          │                                   │
│                          ▼                                   │
│                  ┌──────────────┐                            │
│                  │   Gateway    │                            │
│                  │ ESP32 DevKit │                            │
│                  │   (Wi-Fi)    │                            │
│                  └───────┬──────┘                            │
└──────────────────────────┼───────────────────────────────────┘
                           │ HTTP POST (JSON)
                           │ or USB Serial
┌──────────────────────────▼───────────────────────────────────┐
│                    SERVIDOR (Backend)                         │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────────┐         ┌──────────────────┐            │
│  │   Ingest API    │────────▶│   MySQL DB       │            │
│  │ (PHP/Node.js)   │         │  (leituras_v2)   │            │
│  └─────────────────┘         └──────────────────┘            │
│           │                            │                      │
│           │                            │                      │
│  ┌────────▼────────┐         ┌─────────▼────────┐            │
│  │  Processing     │         │   REST API       │            │
│  │  (Alerts/Calc)  │         │  (Frontend)      │            │
│  └─────────────────┘         └──────────────────┘            │
│                                        │                      │
└────────────────────────────────────────┼──────────────────────┘
                                         │ HTTP/WebSocket
┌────────────────────────────────────────▼──────────────────────┐
│                    FRONTEND (Dashboard)                       │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐ │
│  │  Real-time     │  │   Historical   │  │    Alerts &    │ │
│  │   Dashboard    │  │     Charts     │  │   Settings     │ │
│  └────────────────┘  └────────────────┘  └────────────────┘ │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Data Flow

### 1. Sensor → Gateway (ESP-NOW)

**Frequência:** 30 segundos por nó  
**Protocolo:** Binary packet (SensorPacketV1)  
**Tamanho:** ~30 bytes

```c
typedef struct {
    uint8_t  version;        // 1
    uint8_t  node_id;        // 1-5
    uint8_t  mac[6];         // Source MAC
    uint32_t seq;            // Sequence number
    int16_t  distance_cm;    // HC-SR04 reading
    int16_t  level_cm;       // Calculated level
    uint8_t  percentual;     // 0-100%
    uint32_t volume_l;       // Liters
    int16_t  vin_mv;         // Battery voltage
    int8_t   rssi;           // (filled by gateway)
    uint32_t ts_ms;          // (filled by gateway)
} SensorPacketV1;
```

### 2. Gateway → Backend (HTTP)

**Método:** POST  
**Formato:** JSON  
**Endpoint:** `http://IP:8080/ingest_sensorpacket.php`

**Pipeline interno do gateway:**
```
ESP-NOW callback (ISR)
  ↓ xQueueSendFromISR()
FreeRTOS Queue (10 slots)
  ↓ xQueueReceive()
HTTP Task (task context)
  ↓ esp_http_client_perform()
Backend PHP
```

### 3. Backend → Database (MySQL)

**Tabela:** `leituras_v2`  
**Operação:** INSERT  
**Índice:** (node_id, created_at)

### 4. Backend → Frontend (REST API)

**Endpoints planejados:**
- `GET /api/latest` - Última leitura de cada nó
- `GET /api/history/{node_id}?hours=24` - Histórico
- `GET /api/alerts` - Alertas ativos
- `POST /api/config` - Configurações

## Component Details

### Firmware (ESP32)

**Nodes (ESP32-C3):**
- Sensor: HC-SR04 (ultrasonic distance)
- ADC: Vin monitoring (battery level)
- Radio: ESP-NOW broadcast (channel 11)
- Processing: Integer-only math (no float)
- Storage: NVS for sequence counter
- LED: Status feedback (bootup/tx/error)

**Gateway (ESP32 DevKit V1):**
- Radio: ESP-NOW receive (channel 11)
- Network: Wi-Fi STA (optional) or USB serial
- Queue: Dual-queue ISR→Task pattern
- HTTP: esp_http_client (non-blocking)
- Metrics: Packet counters, RSSI tracking

### Backend (PHP/MySQL)

**Current (Testing):**
- `ingest_sensorpacket.php` - POST endpoint
- `dashboard.php` - Simple HTML table
- `config.php` - DB credentials
- MySQL 5.7+ - Storage

**Future (Production):**
- REST API with endpoints structure
- JWT authentication
- Alert processing engine
- Aggregation queries (avg, trends)
- WebSocket for real-time push

### Frontend (To Be Implemented)

**Framework:** React/Vue/Next.js or TailAdmin template  
**Features:**
- Real-time dashboard (5 node cards)
- Historical charts (level, volume over time)
- Signal quality indicators (RSSI)
- Battery monitoring (vin_mv)
- Alert system (critical levels)
- Export data (CSV/JSON)

## Network Topology

### ESP-NOW Configuration

**Channel:** 11 (fixed, must match Wi-Fi AP)  
**Range:** ~200m line-of-sight, ~50m indoor  
**Topology:** Star (nodes → gateway)  
**MAC:** Broadcast (FF:FF:FF:FF:FF:FF) or unicast

### Wi-Fi Configuration

**Gateway mode:** STA (connects to existing AP)  
**AP requirements:** Fixed channel 11  
**Fallback:** USB serial output for debugging

## Performance Characteristics

### Latency
- Sensor reading: ~60ms (3 samples, median)
- ESP-NOW transmission: <10ms
- Gateway processing: <5ms
- HTTP POST: 50-200ms (network dependent)
- Total: **~125-275ms** (sensor → database)

### Throughput
- 5 nodes × 30s interval = **10 packets/minute**
- Packet size: 30 bytes binary, ~200 bytes JSON
- Network load: **~2KB/min** (negligible)

### Reliability
- ESP-NOW retries: 2 attempts
- Gateway queue: 10 slots (may need 20-30 for 5 nodes)
- HTTP timeout: 3 seconds
- Database: No retry (should add)

## Security Considerations

**Current (Testing):**
- ❌ No authentication
- ❌ No encryption (except Wi-Fi WPA2)
- ❌ No input validation (basic)

**Production Requirements:**
- ✅ HTTPS for backend API
- ✅ JWT token authentication
- ✅ Input sanitization (SQL injection)
- ✅ Rate limiting (prevent DoS)
- ✅ ESP-NOW encryption (optional)

## Scalability

### Current Limits
- **Nodes:** 5 (tested), up to ~20 (ESP-NOW limit)
- **Gateway queue:** 10 slots (bottleneck)
- **Database:** Single table, no partitioning
- **Backend:** Single PHP process (blocking I/O)

### Scale-Up Strategies
- Increase gateway queue to 30+ slots
- Add multiple gateways (geographic distribution)
- Database partitioning by date
- Async backend (Node.js/Python)
- Redis cache for real-time data
- Load balancer for API

## Monitoring & Diagnostics

### Firmware Metrics
- Packet TX count (per node)
- RSSI values (signal quality)
- Sequence gaps (missed packets)
- Battery voltage trends

### Gateway Metrics
- Packets received (total, per node)
- Parse errors
- Queue overflows
- HTTP success/failure rate

### Backend Metrics
- Insert rate (packets/min)
- Query latency
- Error logs
- Storage growth

## Future Enhancements

See `melhorias.md` for detailed plans:
- [ ] Captive portal for gateway config
- [ ] CLI over USB for diagnostics
- [ ] Multi-channel support (dynamic)
- [ ] OTA firmware updates
- [ ] SD card logging (offline mode)
- [ ] OLED display on gateway
- [ ] Alert system (SMS/email/push)
- [ ] Machine learning (anomaly detection)
