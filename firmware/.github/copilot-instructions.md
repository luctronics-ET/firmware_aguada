# Copilot Instructions - Aguada Water Level Telemetry

## Project Overview

ESP32-based water level monitoring system using ESP-NOW for sensor-to-gateway communication. Architecture consists of:
- **Sensor nodes** (ESP32-C3): HC-SR04 ultrasonic sensors measuring water levels, send binary packets via ESP-NOW
- **Gateway** (ESP32 DevKit V1): Receives ESP-NOW packets, adds RSSI/timestamp, forwards to backend via HTTP
- **Backend** (PHP/MySQL, separate repo): Data ingestion and dashboard

**Critical constraint**: All devices must be on **channel 11** for ESP-NOW to work. Nodes and gateway must synchronize channels.

## Build System & Tooling

### ESP-IDF Projects (Primary)
This codebase uses **ESP-IDF** (not Arduino framework for ESP32). Each firmware is a separate ESP-IDF project:

```bash
# Node firmware (ESP32-C3 Supermini)
cd node_ultra1
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# Gateway firmware (ESP32 DevKit V1)
cd gateway_devkit_v1
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

**Important**: The `platformio.ini` file is **legacy/unused**. Ignore it - ESP-IDF is the active toolchain.

### CMake Structure Pattern
All ESP-IDF projects follow this structure:
- Root `CMakeLists.txt`: Minimal, calls `project.cmake` from ESP-IDF
- `main/CMakeLists.txt`: Uses `idf_component_register()` with `INCLUDE_DIRS` pointing to `../..`, `../../components`, `../../common`
- Shared code in `components/` and `common/` directories accessible via relative includes

Example from `node_ultra1/main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "node_ultra1.cpp"
    INCLUDE_DIRS "." "../.." "../../components" "../../common"
)
```

### Build Cleanup
Large `build/` directories (~1.5GB total) are NOT committed. Clean up with:
```bash
cd ~/firmware_aguada
./limpar_builds.sh  # Removes all build/ directories
```

## Core Architecture Patterns

### Binary Telemetry Protocol
All telemetry uses `SensorPacketV1` (`common/telemetry_packet.h`):
- **Packed struct** (no padding) for ESP-NOW transmission
- Node fills: `distance_cm`, `level_cm`, `percentual`, `volume_l`, `vin_mv`, `seq`
- Gateway adds: `rssi`, `ts_ms` (timestamp)
- Version field (`version=1`) for future protocol evolution

**Never use JSON** - this system uses binary packets for efficiency.

### ACK Protocol (Bidirectional Confirmation)
System now implements reliable delivery with ACK confirmation (`common/telemetry_packet.h`):
- **AckPacket structure**: `magic=0xAC`, `version`, `node_id`, `ack_seq`, `rssi`, `status`, `gateway_id`
- **Gateway behavior**: Sends ACK immediately upon receiving telemetry packet
- **Node behavior**: Waits up to 500ms for ACK, retries with exponential backoff if timeout
- **Success tracking**: Nodes maintain `successful_acks/total_attempts` statistics
- **Auto-peer registration**: Gateway automatically registers nodes as ESP-NOW peers for ACK response

Example flow:
1. Node sends `SensorPacketV1` (seq=42)
2. Gateway receives, processes, sends `AckPacket` (ack_seq=42)
3. Node receives ACK within 500ms → Success
4. If no ACK → Retry with next gateway or exponential backoff

### Shared Components (Header-Only Libraries)
Two critical components in `components/`:

1. **`ultrasonic01/ultrasonic01.h`**: HC-SR04 driver
   - `measure_cm()`: Returns distance or -1 on timeout
   - `median3()`: Simple noise rejection
   - Pure inline functions, no `.c` file

2. **`level_calculator/level_calculator.h`**: Tank model calculations
   - `compute()`: Converts distance → level/percentage/volume
   - Uses reservoir model: `VOL_MAX_L=80000`, `LEVEL_MAX_CM=450`, `SENSOR_OFFSET_CM=20`
   - Integer-only math (no floats)

### Configuration: Kconfig + sdkconfig.defaults
Each project uses Kconfig for menuconfig options:
- `Kconfig.projbuild`: Defines custom options (e.g., `CONFIG_LED_ACTIVE_HIGH`)
- `sdkconfig.defaults`: Default overrides (commit this)
- `sdkconfig`: Generated file (gitignored)

Example (`node_ultra1/Kconfig.projbuild`):
```kconfig
config LED_ACTIVE_HIGH
    bool "LED embutido ativo em nivel alto"
    default n
```

Access in code: `#ifdef CONFIG_LED_ACTIVE_HIGH`

## Critical Hardware Configuration

### Gateway MAC Configuration (Redundancy Support)
**Nodes support multiple gateways for failover** (`node_ultra1/main/node_ultra1.cpp`):
```cpp
#define MAX_GATEWAYS 3
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1 (primary)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Gateway 2 (configure with real MAC)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Gateway 3 (configure with real MAC)
};
```

**To add gateway redundancy:**
1. Flash gateway firmware on each device
2. Check serial monitor for "Gateway MAC: XX:XX:XX:XX:XX:XX"
3. Update `GATEWAY_MACS` array in node firmware with real MACs
4. Unconfigured gateways (0xFF:FF:FF:FF:FF:FF) are automatically skipped

**Failover behavior:**
- Node tries last successful gateway first (saved in NVS)
- If fails, round-robin through all configured gateways
- Exponential backoff: 100ms, 200ms, 400ms per gateway
- Automatically switches to working gateway and persists preference

### Channel Configuration
Both node and gateway hardcode channel 11:
```cpp
#define ESPNOW_CHANNEL 11
```

If router uses different channel, either:
- Configure router to channel 11 (preferred)
- Change `ESPNOW_CHANNEL` in both firmwares

### LED Polarity
- **ESP32-C3 Supermini**: Active-low (default)
- **ESP32 DevKit V1**: Active-high (GPIO2)

For active-high LEDs, enable in menuconfig or uncomment in `sdkconfig.defaults`:
```
CONFIG_LED_ACTIVE_HIGH=y
```

## Development Workflows

### Monitor Without Reset
Use `--no-reset` to attach to running node without rebooting:
```bash
idf.py -p /dev/ttyACM0 monitor --no-reset
```

### Backend Integration (Local Development)
Backend runs separately (not in this repo):
```bash
cd ~/firmware_aguada
./start_services.sh  # Starts MySQL + PHP server on port 8080
./status_services.sh # Check if services are running
./stop_services.sh   # Stop PHP server
```

Gateway sends HTTP POST to `http://192.168.0.117:8080/ingest_sensorpacket.php` (configured in `gateway_devkit_v1/main/main.c`).

### Multi-Node Setup
Clone `node_ultra1` to create additional nodes:
- `node_ultra2/` exists as example (identical code)
- Change `node_id` in firmware for each physical device
- All nodes use same gateway MAC and channel

## Code Conventions

### Integer-Only Math
No floating-point arithmetic - everything uses integer operations:
```cpp
int percentual = (level_cm * 100) / level_max_cm;
int64_t vol = ((int64_t)level_cm * (int64_t)vol_max_l) / (int64_t)level_max_cm;
```

### Error Handling
- Distance validation: `MIN_VALID_CM=5`, `MAX_VALID_CM=450`
- Saturation: Level clamped to `[0, LEVEL_MAX_CM]`
- Retries: `ULTRA_SAMPLE_RETRIES=3` with median filtering

### LED Signaling (Nodes)
Minimal LED activity to save power:
- Boot: 3 slow blinks
- Transmitting: 3 fast blinks
- Send failure: Rapid blinking

### NVS Usage
Nodes persist sequence counter in NVS:
```cpp
#define NVS_NAMESPACE "node_cfg"
#define NVS_KEY "seq"
```

## Testing & Debugging

### Serial Output Patterns
**Node logs**: Distance measurements, calculated values, ESP-NOW send status
**Gateway logs**: Received packets, RSSI, HTTP POST results, channel info

### Common Issues
1. **ESP-NOW send fails**: Check channel mismatch (node vs gateway vs router)
2. **No packets received**: Verify gateway MAC address in node firmware
3. **Backend connection fails**: Check `WIFI_SSID`, `WIFI_PASS`, `INGEST_URL` in gateway firmware

### Memory Bank
`.github/` contains chatmode files (architect, code, ask, debug) - these are for AI agent context, not runtime code.

## Legacy Components
- `arduino/nano_ethernet_ultra/`: Old Arduino Nano Ethernet implementation (inactive)
- `platformio.ini`: Legacy config (unused, ESP-IDF is active toolchain)
- `doc/`: Doxygen-generated docs (if present)

## When Adding New Features

1. **New sensor types**: Add header-only component in `components/`
2. **Protocol changes**: Increment `SENSOR_PACKET_VERSION`, update `telemetry_packet.h`
3. **New nodes**: Clone existing node project, change `node_id` and project name
4. **Backend changes**: Update `INGEST_URL` and backend repo separately

Always test channel synchronization and MAC address configuration first when issues arise.
