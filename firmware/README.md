# Telemetria ESP-NOW — Nível de Água (ESP32)

Sistema de telemetria com sensores ultrassônicos ESP32-C3, gateway ESP32 DevKit V1, e backend PHP/MySQL.

## ⚡ Performance & Limpeza

**Computador lento?** O projeto pode ter ~1.5GB em pastas `build/`. Execute:

```bash
cd ~/firmware_aguada
./limpar_builds.sh  # Remove builds antigos, recupera espaço
```

Os builds são recriados automaticamente ao compilar (`idf.py build`). Nunca commit builds no Git!

## 🚀 Inicialização Rápida

**Iniciar todo o sistema automaticamente:**

```bash
cd ~/firmware_aguada
./start_services.sh  # Inicia MySQL + PHP + abre navegador
```

**Verificar status:**
```bash
./status_services.sh  # Mostra status de todos os serviços
```

**Parar serviços:**
```bash
./stop_services.sh   # Para servidor PHP
```

O script `start_services.sh` faz automaticamente:
- ✅ Verifica/inicia MySQL
- ✅ Cria banco `sensores_db` (se não existir)
- ✅ Importa schema (se necessário)
- ✅ Inicia servidor PHP na porta 8080
- ✅ Abre navegador em http://localhost:8080

## 📁 Estrutura
- `node_ultra1/`: firmware do nó (ESP32-C3 por padrão).
- `node_ultra2/`: segundo nó (clone do Ultra01).
- `node_cie_dual/`: **NOVO!** Firmware para 2 sensores HC-SR04 (cisterna CIE com 2 reservatórios independentes).
- `gateway_devkit_v1/`: firmware do gateway (ESP32 DevKit V1, fila HTTP opcional).
- `components/` e `common/`: código compartilhado (`ultrasonic01`, `level_calculator`, `telemetry_packet.h`).
- `backend/`: Backend PHP/MySQL para ingestão e dashboard.
- `frontend/`: Estrutura preparada para dashboard web (React/Vue/Next.js).
- `database/`: Schemas SQL e migrations.
- `docs/`: Documentação de arquitetura.

**Arquivos importantes:**
- `.gitignore`: Evita commit de builds (configurado automaticamente)
- `limpar_builds.sh`: Script para liberar espaço em disco
- `REORGANIZACAO.md`: Plano de estrutura futura (firmware/backend/frontend separados)

## Parâmetros do reservatório (Node Ultra01)
- `VOL_MAX_L = 80000` (80 m³)
- `LEVEL_MAX_CM = 450`
- `SENSOR_OFFSET_CM = 20`
- Resolução: 1 cm para nível; volume proporcional ao nível.

## Pinos (ESP32-C3 Supermini por padrão)
- `TRIG_GPIO = GPIO_NUM_1`
- `ECHO_GPIO = GPIO_NUM_0`
- `LED_GPIO = GPIO_NUM_8` (LED embutido)
- Ajuste conforme seu hardware.

### Ativo-alto vs ativo-baixo do LED
- Por padrão, considera LED ativo-baixo (Supermini). Se seu LED for ativo-alto, rode `idf.py menuconfig` no projeto `node_ultra1` e habilite `Node Ultra01 Options -> LED embutido ativo em nivel alto`, ou descomente `CONFIG_LED_ACTIVE_HIGH=y` em `node_ultra1/sdkconfig.defaults`.

## Status de LED (nó)
- LED fica apagado na maior parte do tempo (padrão).
- Durante inicialização/radio up ("procurando gateway"), 3 piscadas lentas.
- Ao transmitir, 3 piscadas curtas.
- Se falhar o envio, piscadas rápidas (erro).

## Formatos de Pacote (ESP-NOW)

### SensorPacketV1 - Estrutura Fixa (28 bytes)
Pacote otimizado para sensores ultrassônicos de nível:
- `version=1`, `node_id`, `mac[6]`, `seq`
- `distance_cm`, `level_cm`, `percentual`, `volume_l`, `vin_mv`
- `flags`, `alert_type` (detecção de anomalias)
- `rssi`, `ts_ms` (preenchidos pelo gateway)

**Ideal para**: Sensores padronizados com alta frequência de envio

### GenericPacket - Estrutura Variável (20-250 bytes)
Pacote flexível com pares chave-valor para dados arbitrários:
- `magic=0xDA`, `version=2`, `node_id`, `mac[6]`, `seq`
- `pair_count` (0-10 pares)
- Array de pares: `[label]:[type]:[value]`
- Suporta 9 tipos: int8/16/32, uint8/16/32, float, bool, string

**Ideal para**: Prototipagem, sensores diversos, dados heterogêneos

📖 **Veja exemplos completos em**: `GENERIC_PACKET_EXAMPLE.md`

### aguadaUltrasonic01 - Ultra-Minimal (13 bytes)
Pacote minimalista com processamento server-side:
- `magic=0xA1`, `version=1`, `node_id`
- `distance_cm` (único dado transmitido)
- `flags` (low_battery, sensor_error)
- `rssi`, `ts_ms` (preenchidos pelo gateway)

**Processamento**: Servidor calcula `level_cm`, `percentual`, `volume_l` com base em configuração do `node_id` armazenada em banco de dados

**Vantagens**:
- ✅ 54% menor que SensorPacketV1 (13 vs 28 bytes)
- ✅ Calibração remota sem reflash
- ✅ Ideal para redes com 50+ sensores
- ✅ Menor consumo de bateria

**Limitações**:
- ❌ Requer backend funcional (não funciona offline)
- ❌ Latência adicional +20-50ms (processamento server)

📖 **Documentação completa em**: `AGUADA_ULTRA01_PACKET.md`

## Redundância de Gateways (v2.0+)

**Novo recurso**: Nós agora suportam até 3 gateways simultâneos para alta disponibilidade!

### Configuração
```cpp
// Em node_ultra1/main/node_ultra1.cpp
#define MAX_GATEWAYS 3
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1
    {0x24, 0x0a, 0xc4, 0x9a, 0x58, 0x28},  // Gateway 2 (exemplo)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Gateway 3 (não configurado)
};
```

### Funcionamento
1. **Preferência inteligente**: Nó tenta primeiro o último gateway bem-sucedido (salvo em NVS)
2. **Failover automático**: Se falhar, tenta os outros gateways em round-robin
3. **Retry com backoff**: 3 tentativas por gateway com delays exponenciais (100ms → 200ms → 400ms)
4. **Persistência**: Gateway bem-sucedido é salvo para próxima transmissão

## Protocolo ACK Bidirecional (v2.1+)

**Confirmação de entrega**: Sistema agora garante que dados chegaram ao gateway!

### Como Funciona
1. **Nó envia telemetria** (`SensorPacketV1`) via ESP-NOW
2. **Gateway recebe e processa** dados
3. **Gateway envia ACK** imediatamente (`AckPacket` com seq confirmado)
4. **Nó espera ACK** por até 500ms
5. **Se ACK recebido**: ✅ Sucesso confirmado, salva gateway preferido
6. **Se timeout**: ⚠️ Tenta próximo gateway ou retry exponencial

### Estrutura do ACK
```cpp
typedef struct {
    uint8_t  magic;       // 0xAC (validação)
    uint8_t  version;     // 1
    uint8_t  node_id;     // ID do nó confirmado
    uint32_t ack_seq;     // Sequência confirmada
    int8_t   rssi;        // RSSI medido pelo gateway
    uint8_t  status;      // 0=OK, 1=enfileirado, 2=erro
    uint8_t  gateway_id;  // Qual gateway enviou (0-2)
} AckPacket;
```

### Métricas em Tempo Real
Nó rastreia taxa de sucesso:
```
I (5678) node_ultra01: ✓ Sent successfully to gateway 0 (retry 0) with ACK confirmation
I (5690) node_ultra01: 📊 Stats: 42/45 successful (93.3% success rate)
```

### Logs
```
I (1234) node_ultra01: Trying gateway 0: 80:F3:DA:62:A7:84
I (1250) node_ultra01: ✓ ACK recebido: seq=42, rssi=-45, gateway=0, status=0
I (1260) node_ultra01: ✓ Sent successfully to gateway 0 (retry 0) with ACK confirmation
I (1270) node_ultra01: 📊 Stats: 10/10 successful (100.0% success rate)
```

### Logs (Gateway)
```
I (5678) AGUADA_GATEWAY: ✓ Nó auto-registrado: 24:0A:C4:9A:58:28
D (5680) AGUADA_GATEWAY: ✓ ACK enviado para seq=42
```

## Fila Persistente NVS (v2.2+)

**Gateway não perde dados se backend cair!**

### Características
- ✅ **Circular buffer de 50 pacotes** em NVS Flash
- ✅ **Persiste através de reboots** do gateway
- ✅ **Prioriza backlog**: Ao reconectar, envia dados salvos primeiro
- ✅ **Flag is_backlog**: Backend sabe se dado é histórico ou tempo real

### Funcionamento
1. **Backend online**: Gateway envia telemetria normalmente via HTTP
2. **Backend offline**: Pacotes são salvos em NVS (até 50)
3. **Backend volta**: Gateway processa backlog completo antes de dados novos
4. **Fila cheia**: Descarta pacote mais antigo (FIFO)

### JSON Serialization
```json
{
  "version": 1,
  "node_id": 1,
  "seq": 42,
  "level_cm": 321,
  "volume_l": 56890,
  "is_backlog": true,
  "ts_ms": 1702752345
}
```

### Logs
```
I (1234) AGUADA_GATEWAY: 📦 Fila NVS inicializada: 3 pacotes pendentes (head=0 tail=3)
I (5678) AGUADA_GATEWAY: ⏳ Aguardando IP para enviar backlog...
I (8901) AGUADA_GATEWAY: 📤 HTTP backlog status: 200
I (8910) AGUADA_GATEWAY: ✓ Pacote do backlog enviado com sucesso
I (9123) AGUADA_GATEWAY: ✓ Backlog NVS vazio - processando telemetria em tempo real
```

## Sincronização de Tempo - SNTP (v2.2+)

**Timestamps reais ao invés de milliseconds-since-boot!**

### Configuração
- **Servidores NTP**: `pool.ntp.br`, `a.st1.ntp.br`
- **Timezone**: Brasil (BRT3 = UTC-3)
- **Fallback**: Se SNTP não sincronizado, usa `esp_timer_get_time()`

### Campo ts_ms
- **Antes**: Milissegundos desde boot (`esp_timer_get_time() / 1000`)
- **Agora**: Segundos UNIX epoch (timestamp absoluto desde 1970-01-01)

### Logs
```
I (1234) AGUADA_GATEWAY: Inicializando SNTP...
I (1250) AGUADA_GATEWAY: SNTP iniciado (aguardando sincronização...)
I (5678) AGUADA_GATEWAY: ⏰ SNTP sincronizado: 2025-12-16 14:23:45
```

## Filtro de Kalman 1D (v2.3+)

**Precisão melhorada de ±1cm para ±0.3cm (estimativa)!**

### Características
- ✅ **Substitui median3()** por estimativa Bayesiana
- ✅ **Parâmetros**: process_noise=1.0, measurement_noise=2.0
- ✅ **Mantém estado entre leituras** (convergência gradual)
- ✅ **Reset automático** em caso de falha total do sensor

### Implementação
```cpp
// Em components/ultrasonic01/ultrasonic01.h
class KalmanFilter {
    float x;  // Estado estimado (distância)
    float p;  // Covariância do erro de estimativa
    float q;  // Covariância do ruído de processo
    float r;  // Covariância do ruído de medição
    bool initialized;
public:
    float update(int measurement_cm);  // Retorna valor filtrado
    void reset();
};
```

### Logs
```
I (1234) node_ultra01: Reading 0: raw=123cm filtered=123cm
I (1300) node_ultra01: Reading 1: raw=125cm filtered=124cm
I (1366) node_ultra01: Reading 2: raw=122cm filtered=123cm
I (1432) node_ultra01: meas: distance=123 cm, level=327 cm, pct=73%, vol=58133 L
```

## Detecção de Anomalias (v2.3+)

**Sistema detecta 3 tipos de anomalias em tempo real no edge (nó):**

1. **Rapid Drop** (vazamento): Nível cai ≥50cm rapidamente
2. **Rapid Rise** (falha de bomba/inundação): Nível sobe ≥50cm rapidamente  
3. **Sensor Stuck** (sensor travado): Sem mudança ≥2cm por 120 minutos

### Campos Adicionados
```cpp
typedef struct {
    // ... campos existentes ...
    uint8_t  flags;       // Bit 0: is_alert
    uint8_t  alert_type;  // 0=none, 1=rapid_drop, 2=rapid_rise, 3=sensor_stuck
} SensorPacketV1;
```

### Logs
```
I (1234) node_ultra01: 🚨 ALERTA: Queda rápida detectada! Δ=-75cm (possível vazamento)
I (1250) node_ultra01: ⚠️ Pacote marcado como alerta (tipo=1)
```

Gateway exibe com destaque visual:
```
W (5678) AGUADA_GATEWAY: 🚨 ══════════════════════════════════════════════════
W (5680) AGUADA_GATEWAY: 🚨 ALERTA DETECTADO! Tipo: 1 (Rapid Drop - Vazamento)
W (5682) AGUADA_GATEWAY: 🚨 Node: 1, Level: 252cm, Flags: 0x01
W (5684) AGUADA_GATEWAY: 🚨 ══════════════════════════════════════════════════
```

## 🗺️ Mapeamento dos Reservatórios

Sistema completo com **5 nodes** monitorando **730.000 litros** (730m³):

### Nodes Padrão (80.000L cada) - Firmware: node_ultra1
| Node ID | Código | Localização | Capacidade | Sensor |
|---------|--------|-------------|------------|---------|
| 1 | **RCON** | Castelo - Reservatório de Consumo | 80.000L | 1× HC-SR04 |
| 2 | **RCAV** | Castelo - Reservatório de Incêndio | 80.000L | 1× HC-SR04 |
| 3 | **RCB3** | Bloco 3 - Reservatório Geral | 80.000L | 1× HC-SR04 |

**Pinout padrão**: TRIG→GPIO1, ECHO→GPIO0, Altura: 450cm, Offset: 20cm

📄 **Documentação**: `node_ultra1/NODES_SETUP_GUIDE.md`  
🗄️ **Configuração SQL**: `node_ultra1/nodes_config.sql`

### Cisterna CIE (245.000L cada) - Firmware: node_cie_dual
| Node ID | Código | Localização | Capacidade | Sensores |
|---------|--------|-------------|------------|----------|
| 4 | **CIE1** | Cisterna Ilha Engenho 01 | 245.000L | HC-SR04 #1 |
| 5 | **CIE2** | Cisterna Ilha Engenho 02 | 245.000L | HC-SR04 #2 |

**Arquitetura especial**: 1 ESP32 com 2 sensores (dual-sensor node)  
**Pinout**: Sensor1 (GPIO1/0) + Sensor2 (GPIO3/2)

📄 **Documentação**: `node_cie_dual/README.md`, `IMPLEMENTATION_SUMMARY.md`  
🗄️ **Configuração SQL**: `node_cie_dual/backend_config.sql`  
🎨 **Dashboard**: `node_cie_dual/DASHBOARD_DESIGN.md`

### Capacidade Total
```
Castelo + RCB3:  240.000L (3 nodes × 80.000L)
CIE (dual):      490.000L (2 reservatórios × 245.000L)
──────────────────────────────────────────────────
TOTAL:           730.000L (730m³)
```

---

## Multi-Sensor Fusion - node_cie_dual (v2.4+)

**Firmware para cisterna CIE com 2 reservatórios independentes lado a lado!**

### Características
- ✅ **1 ESP32 = 2 sensores HC-SR04** medindo reservatórios separados
- ✅ **Sensor 1**: GPIO1/GPIO0 → envia como `node_id=4` (CIE1)
- ✅ **Sensor 2**: GPIO3/GPIO2 → envia como `node_id=5` (CIE2)
- ✅ **Leitura e envio independente**: 2 pacotes por ciclo
- ✅ **Filtro Kalman por sensor**: Estado persistente separado
- ✅ **Detecção de anomalias individual**: Baseline independente
- ✅ **Tratamento de erro robusto**: Um sensor pode falhar, outro continua
- ✅ **Sequências NVS separadas**: `seq1` e `seq2`
- ✅ **Delay inter-sensor**: 100ms para evitar interferência GPIO
- ✅ **Capacidade real**: 245.000L por reservatório (vs 80.000L padrão)
- ✅ **Backend configurado**: SQL com MACs diferenciados
- ✅ **Dashboard design**: Cards compactos com visualização in/out

### Build & Flash
```bash
cd ~/firmware_aguada/firmware/node_cie_dual
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Logs Exemplo
```
I (1234) node_cie_dual: ═══════════════════════════════════════════
I (1235) node_cie_dual: 📊 CIE1 (node_id=4) - Medição #42
I (1236) node_cie_dual: ═══════════════════════════════════════════
I (1240) node_cie_dual: ✅ CIE1: Distância final (Kalman): 123cm (3/3 leituras válidas)
I (1365) node_cie_dual: ✅ CIE1: Pacote enviado com sucesso (seq=42)

I (1465) node_cie_dual: ═══════════════════════════════════════════
I (1466) node_cie_dual: 📊 CIE2 (node_id=5) - Medição #38
I (1467) node_cie_dual: ═══════════════════════════════════════════
I (1580) node_cie_dual: ✅ CIE2: Pacote enviado com sucesso (seq=38)
```

📖 **Documentação completa**: `node_cie_dual/README.md`

**Edge AI: Sistema detecta vazamentos, inundações e sensores travados!**

### Tipos de Alerta
- **ALERT_RAPID_DROP (1)**: Vazamento - nível cai ≥50cm em um intervalo
- **ALERT_RAPID_RISE (2)**: Bomba quebrada/inundação - nível sobe ≥50cm
- **ALERT_SENSOR_STUCK (3)**: Sensor travado - sem mudança >2cm por 120 minutos

### Campos Adicionados ao Pacote
```cpp
typedef struct {
    // ... campos existentes ...
    uint8_t flags;       // Bit 0: FLAG_IS_ALERT
    uint8_t alert_type;  // 0=none, 1=drop, 2=rise, 3=stuck
} SensorPacketV1;
```

### Funcionamento
1. **Baseline inicial**: Primeira leitura estabelece nível base
2. **Monitoramento contínuo**: Calcula delta entre leituras
3. **Detecção de anomalia**: Se thresholds excedidos, marca pacote
4. **Transmissão imediata**: Alertas não aguardam intervalo normal

### Logs (Nó)
```
I (1234) node_ultra01: 🎯 Anomaly detection initialized (baseline=327cm)
...
W (5678) node_ultra01: 🚨 ALERTA: Queda rápida detectada! Δ=-52cm (possível vazamento)
I (5690) node_ultra01: ⚠️ Pacote marcado como alerta (tipo=1)
```

### Logs (Gateway)
```
W (8901) AGUADA_GATEWAY: ╔════════════════════════════════════════════════════╗
W (8910) AGUADA_GATEWAY: ║          🚨 ALERTA DE ANOMALIA DETECTADO 🚨       ║
W (8920) AGUADA_GATEWAY: ╠════════════════════════════════════════════════════╣
W (8930) AGUADA_GATEWAY: ║ Tipo: RAPID_DROP
W (8940) AGUADA_GATEWAY: ║ Nó ID: 1 | Sequência: 42
W (8950) AGUADA_GATEWAY: ╚════════════════════════════════════════════════════╝
```

### JSON Serialization
```json
{
  "version": 1,
  "node_id": 1,
  "seq": 42,
  "level_cm": 275,
  "flags": 1,
  "alert_type": 1,
  "is_backlog": false
}
```

### Logs (Gateway)
```
I (5678) AGUADA_GATEWAY: ✓ Nó auto-registrado: 24:0A:C4:9A:58:28
D (5680) AGUADA_GATEWAY: ✓ ACK enviado para seq=42
```

### Logs
```
I (1234) node_ultra01: Total gateways configured: 2
I (1245) node_ultra01: Last successful gateway: 0 (80:F3:DA:62:A7:84)
I (5678) node_ultra01: Trying gateway 0: 80:F3:DA:62:A7:84
I (5690) node_ultra01: ✓ Sent successfully to gateway 0 (retry 0)
```

Se gateway 0 falhar:
```
E (5700) node_ultra01: ✗ Gateway 0 failed after 3 retries
I (5710) node_ultra01: Trying gateway 1: 24:0A:C4:9A:58:28
I (5720) node_ultra01: ✓ Sent successfully to gateway 1 (retry 1)
I (5730) node_ultra01: Gateway failover: 0 -> 1
```

Tabela simples (raw recebido pelo gateway):

| mac | node_id | seq | version | distance_cm | level_cm | percentual (%) | volume_l | vin_mv | rssi | ts_ms |
|---|---|---|---|---|---|---|---|---|---|---|
| FF:FF:FF:FF:FF:FF | 1 | 42 | 1 | 123 | 321 | 71 | 56890 | 3300 | -60 | 1234567 |

## Build (ESP-IDF)
Apps separados com CMake de projeto:

- Nó: `node_ultra1/`
	```bash
	cd node_ultra1
	idf.py set-target esp32c3   # ou esp32, conforme o hardware
	idf.py menuconfig           # opcional
	idf.py build
	idf.py -p /dev/ttyUSB0 flash monitor
	```

- Gateway: `gateway_devkit_v1/`
	```bash
	cd gateway_devkit_v1
	idf.py set-target esp32     # esp32/esp32c3 conforme hardware
	idf.py build
	idf.py -p /dev/ttyUSB1 flash monitor
	```

Estrutura padrão: `project.cmake` no `CMakeLists.txt` raiz e registros em `main/CMakeLists.txt` via `idf_component_register`. Includes adicionam `../..`, `../../components` e `../../common`.



## Próximos Passos (Expansão)
- Filtro de média/mediana (já há mediana de 3 leituras). Pode-se aumentar janela e implementar EMA/Kalman.
- Tratamento de erros (intervalo válido de distância, saturação já aplicada).
- Display I2C no nó (LCD/OLED) usando os resultados de `level_calculator`.
- Integração com servidor via Wi-Fi (HTTP/MQTT) no gateway.

## Observações
- O código do nó prioriza operação autônoma: mede, calcula e envia, depois deep-sleep.
- Se necessário, altere `SAMPLE_INTERVAL_S` para o período desejado.
- Caso queira manter JSON, basta recuperar a versão anterior (antes da migração para pacote binário) ou implementar um `#define` para alternar o formato.

## Monitoramento Serial
Para monitorar a saída serial do nó Ultra01 sem reiniciá-lo, use o seguinte comando:

```bash
cd ~/firmware_aguada/node_ultra1
idf.py -p /dev/ttyACM0 monitor --no-reset
```

Isso é útil para depuração e verificação de dados em tempo real.
