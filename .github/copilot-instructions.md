# Firmware Aguada - AI Coding Agent Instructions

## Project Overview

ESP32-based **water level telemetry system** using ESP-NOW wireless protocol. Sensor nodes measure ultrasonic distance to water surface and transmit compact binary packets to a gateway, which forwards data via HTTP to a PHP/MySQL backend.

**Current Production Setup:**
- **4x node_ultra1** + **1x node_ultra2**: ESP32-C3 sensor nodes deployed in field
- **1x gateway_devkit_v1**: ESP32 DevKit V1 receiving ESP-NOW, forwarding via Wi-Fi or USB
- **Backend**: PHP/MySQL for data ingestion and processing
- **Stage**: Testing sensor→gateway→backend pipeline before frontend integration

**Project Structure (Reorganized):**
```
firmware_aguada/
├── node_ultra1/        # ESP32-C3 sensor firmware
├── node_ultra2/        # Second sensor firmware
├── gateway_devkit_v1/  # ESP32 gateway firmware
├── components/         # Shared C++ modules (ultrasonic01, level_calculator)
├── common/             # Shared headers (telemetry_packet.h)
├── backend/            # PHP/MySQL ingestion + dashboard
├── frontend/           # Dashboard web (em desenvolvimento)
├── database/           # SQL schemas + migrations
└── docs/               # Architecture documentation
```

**Key Components:**
- `node_ultra1/`, `node_ultra2/`: ESP32-C3 sensor nodes (ultrasonic HC-SR04, ADC voltage monitoring, ESP-NOW TX)
- `gateway_devkit_v1/`: ESP32 DevKit V1 gateway (ESP-NOW RX → HTTP POST via queue-based pipeline)
- `common/telemetry_packet.h`: Binary packet format (SensorPacketV1, packed struct)
- `components/`: Reusable modules (ultrasonic01, level_calculator)
- `backend/`: PHP ingest endpoint + MySQL schema (testing/staging environment)
- `frontend/`: Dashboard web preparado para React/Vue/Next.js
- `database/`: SQL schemas e queries úteis

## Critical Architecture Patterns

### 1. Queue-Based ISR → Task Pipeline (Gateway)

**Problem:** Cannot call blocking LwIP operations (HTTP, sockets) from ESP-NOW callback (ISR context).  
**Solution:** Two-stage queue pattern documented in `gateway_devkit_v1/ARCHITECTURE.md`:

```c
espnow_recv_cb (ISR) → xQueueSendFromISR() → FreeRTOS Queue → http_post_task (task context) → esp_http_client_perform()
```

**Always follow this pattern when adding network operations to gateway.** Never call blocking network APIs from ESP-NOW callbacks.

### 2. Packed Binary Protocol

`SensorPacketV1` uses `__attribute__((packed))` to avoid padding differences across compilers. Fields:
- Node-filled: `version`, `node_id`, `mac[6]`, `seq`, `distance_cm`, `level_cm`, `percentual`, `volume_l`, `vin_mv`
- Gateway-filled: `rssi`, `ts_ms`

**When modifying protocol:** Update struct in `common/telemetry_packet.h`, increment `SENSOR_PACKET_VERSION`, update both node and gateway parsers, and backend schema.

### 3. Integer-Only Calculations

No floating-point math on nodes (efficiency). Use integer arithmetic with explicit units:
- Distance/level in cm (1 cm resolution)
- Voltage in mV
- Percentual 0-100 (integer)
- Volume in liters (scaled via `(level_cm * vol_max_l) / level_max_cm`)

See `components/level_calculator/level_calculator.h` for reference implementation.

## Build System

**Dual build support:** ESP-IDF + PlatformIO (use whichever works better for your workflow).

### ESP-IDF
```bash
cd node_ultra1
idf.py set-target esp32c3        # or esp32 for gateway
idf.py menuconfig                 # configure LED polarity, pins, MAC addresses, etc.
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

**Monitor without reset:** `idf.py -p /dev/ttyACM0 monitor --no-reset` (useful for observing periodic telemetry without restarting node).

### PlatformIO (Arduino framework)
```bash
# Build gateway
pio run -e gateway
pio run -e gateway -t upload

# Build node
pio run -e node_ultra1
pio run -e node_ultra1 -t upload
```

**⚠️ Deployed devices:** 5 nodes already flashed and operational. When modifying firmware, consider:
- Backward compatibility with `SensorPacketV1` protocol
- OTA update strategy or physical reflash plan
- Testing changes on spare hardware before field deployment

## Component Structure

Reusable code lives in `components/` (ESP-IDF) and `common/`:

- **ultrasonic01**: Header-only C++ namespace for HC-SR04. Returns cm or -1 on timeout.
- **level_calculator**: Header-only namespace. `compute(distance_cm, Model)` → `Result{level_cm, percentual, volume_l}`.
- **telemetry_packet.h**: Shared packet definition (included by both node and gateway).

**CMakeLists.txt pattern for nodes/gateway:**
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "." "../.." "../../components" "../../common"
)
```

## Configuration Guidelines

### Node (`node_ultra1/main/node_ultra1.cpp`, `node_ultra2/main/*.cpp`)
- Hardware pins: `TRIG_GPIO`, `ECHO_GPIO`, `ADC_CHANNEL_VIN`, `LED_GPIO`
- Tank model: `VOL_MAX_L`, `LEVEL_MAX_CM`, `SENSOR_OFFSET_CM`
- ESP-NOW: `GATEWAY_MAC`, `ESPNOW_CHANNEL` (must match gateway)
- **Node ID**: Each deployed node must have unique `node_id` (1-5 for current deployment)
- LED polarity: Configure via `idf.py menuconfig` → Node Ultra01 Options → LED active high/low

### Gateway (`gateway_devkit_v1/main/main.c`)
- Wi-Fi: `WIFI_SSID`, `WIFI_PASS`
- Backend: `INGEST_URL` (PHP endpoint, e.g., `http://192.168.0.117:8080/ingest_sensorpacket.php`)
- ESP-NOW: `ESPNOW_CHANNEL` (must match all nodes)
- Queue sizes: `espnow_queue` (10 slots - may need increase for 5 concurrent nodes), `http_queue`
- **Dual connectivity**: Can forward via Wi-Fi (HTTP) or USB serial (for debugging/testing)

**Channel synchronization critical:** Gateway operates on Wi-Fi AP's channel. Set AP to fixed channel 11 (or update `ESPNOW_CHANNEL` everywhere). See `melhorias.md` for multi-channel considerations.

## LED Status Patterns (Node)

Implemented via non-blocking `esp_timer` callbacks:
- **Bootup/Radio Init:** 3 slow blinks (500ms period)
- **Transmitting:** 3 fast blinks (150ms period)
- **Error:** Rapid blinks (100ms period)
- **Idle:** LED off (default low-power state)

## Common Workflows

### Testing Pipeline Integrity (Current Priority)
**Goal:** Verify sensors → gateway → database → backend processing chain

1. **Monitor node transmission:**
   ```bash
   cd node_ultra1
   idf.py -p /dev/ttyACM0 monitor --no-reset
   ```
   Look for: distance_cm, level_cm, seq incrementing, "ESP-NOW sent" confirmation

2. **Monitor gateway reception:**
   ```bash
   cd gateway_devkit_v1
   idf.py -p /dev/ttyACM1 monitor
   ```
   Look for: MAC addresses of all 5 nodes, RSSI values, HTTP POST success/failure

3. **Verify database ingestion:**
   ```sql
   SELECT node_id, COUNT(*), MAX(created_at) 
   FROM leituras_v2 
## Backend Integration

PHP/MySQL stack in `backend/` (testing/staging environment):
- **schema.sql**: Moved to `database/schema.sql`
- **ingest_sensorpacket.php**: Accepts JSON POST, validates, inserts row
- **dashboard.php**: Basic HTML table of recent readings
- **config.php**: DB credentials (hostname, user, password, database)

**Current Focus:** Validate complete data pipeline before frontend integration:
1. Sensors transmit correctly (distance, level, volume calculations)
2. Gateway receives all 5 nodes reliably (no packet loss)
3. Database stores raw telemetry (all fields, proper timestamps)
4. Backend processes/calculates derived metrics (if any)
5. Data availability for future frontend consumption (REST API, GraphQL, etc.)

**Security:** No authentication in current testing phase. Add token-based auth before production deployment.

**Frontend:** Structure prepared in `frontend/` for React/Vue/Next.js or TailAdmin template integration.

### Debugging Multi-Node Issues
- **Missing nodes:** Check `node_id`, `GATEWAY_MAC` matches, channel sync, power/range
- **Gateway queue overflow:** Increase `espnow_queue` from 10 to 20-30 for 5 concurrent nodes
- **Packet collisions:** Nodes transmit every 30s; add random jitter (±5s) to avoid simultaneous sends
- **HTTP backlog:** Monitor gateway's HTTP task queue; consider increasing timeout or adding retry logic

### Modifying Telemetry Fields (Breaking Change)
⚠️ **Requires reflashing all 5 deployed nodes**

1. Edit `common/telemetry_packet.h` (preserve `packed` attribute)
2. Increment `SENSOR_PACKET_VERSION` (e.g., 1 → 2)
3. Update node's packet-building code (all node_ultra1/ultra2)
4. Update gateway's packet-parsing code
5. Modify `backend_xampp/schema.sql` and `ingest_sensorpacket.php`
6. Test with 1 node before mass reflash
7. Plan field visit to reflash remaining 4 nodess`
- Monitor queue overflow: Increase `espnow_queue` size if drops occur
- HTTP failures: Check `INGEST_URL`, backend logs, and network connectivity

## Backend Integration

Simple PHP/MySQL stack in `backend_xampp/`:
- **schema.sql**: `leituras_v2` table (auto-increment ID, timestamp, all packet fields)
- **ingest_sensorpacket.php**: Accepts JSON POST, validates, inserts row
- **dashboard.php**: Basic HTML table of recent readings
- **config.php**: DB credentials (hostname, user, password, database)

**No authentication:** Deploy only on trusted networks or add token-based auth.

## Project Structure & Performance

**⚠️ Build Artifacts:** ESP-IDF creates large `build/` directories (~180MB each). These should NEVER be committed to Git.

**Performance tip:** If VS Code is slow, run cleanup:
```bash
cd ~/firmware_aguada
./limpar_builds.sh  # Removes all build/ folders, recovers ~1.5GB
```

After cleanup, rebuild only when flashing:
```bash
cd node_ultra1
idf.py build  # Recreates build/ when needed
```

**Quick Start:** Initialize database and backend automatically:
```bash
cd ~/firmware_aguada
./start_services.sh    # Starts MySQL + PHP server + opens browser
./status_services.sh   # Check services status
./stop_services.sh     # Stop PHP server
```

## Key Files Reference

| File | Purpose |
|------|---------|
| `common/telemetry_packet.h` | Binary packet protocol (single source of truth) |
| `gateway_devkit_v1/ARCHITECTURE.md` | Queue-based pipeline explanation + crash fix |
| `melhorias.md` | Future improvements (portal cativo, CLI, multi-channel) |
| `node_ultra1/main/node_ultra1.cpp` | Main sensor node logic (measure → compute → transmit loop) |
| `gateway_devkit_v1/main/main.c` | Gateway ISR → queue → HTTP POST implementation |
| `components/ultrasonic01/ultrasonic01.h` | HC-SR04 driver (header-only, returns cm) |
| `components/level_calculator/level_calculator.h` | Tank level/volume calculator (integer math) |
| `.gitignore` | Prevents committing builds, logs, and temp files |
| `limpar_builds.sh` | Script to clean build artifacts and free space |
| `backend/README.md` | Backend PHP/MySQL documentation |
| `database/README.md` | Database schema and useful queries |
| `frontend/README.md` | Frontend structure and integration guide |
| `docs/ARCHITECTURE.md` | Complete system architecture documentation |
| `docs/EXPANSION_GUIDE.md` | Guide for adding sensors and dynamic configuration |

## Expansion & Configuration

- **Serial monitoring:** Use `idf.py monitor --no-reset` to avoid restarting node during observation
- **Node deep sleep:** Currently active loop mode (`vTaskDelay`); can re-enable deep sleep by replacing loop with `esp_deep_sleep_start()`
- **Gateway queue depth:** Default 10 slots - **likely needs increase to 20-30 for 5 concurrent nodes**
- **Median filtering:** Node takes 3 ultrasonic samples and uses median (configurable via `ULTRA_SAMPLE_RETRIES`)
- **Multi-node testing:** With 5 nodes transmitting every 30s, expect ~10 packets/minute at gateway
- **USB vs Wi-Fi:** Gateway can output via serial (debugging) or HTTP POST (production); both modes useful for validation

## Testing Notes

- **Serial monitoring:** Use `idf.py monitor --no-reset` to avoid restarting node during observation
- **Node deep sleep:** Currently active loop mode (`vTaskDelay`); can re-enable deep sleep by replacing loop with `esp_deep_sleep_start()`
- **Gateway queue depth:** Default 10 slots; monitor for overflows if multiple nodes transmit simultaneously
- **Median filtering:** Node takes 3 ultrasonic samples and uses median (configurable via `ULTRA_SAMPLE_RETRIES`)

## Anti-Patterns to Avoid

❌ Calling `esp_http_client_perform()` or socket APIs from ESP-NOW callback  
❌ Using floating-point on nodes (use integer mV/cm/liters)  
❌ Mismatched `ESPNOW_CHANNEL` between nodes and gateway  
❌ Forgetting `__attribute__((packed))` when modifying SensorPacketV1  
❌ Hardcoding MAC addresses in multiple places (use `#define GATEWAY_MAC`)  
❌ Blocking delays in ISR context (`vTaskDelay` requires task context)
