# Node CIE Dual - Dual Sensor Firmware

ESP32-C3 firmware para **2 sensores HC-SR04** medindo **2 reservatórios independentes** na cisterna CIE.

## 🏗️ Arquitetura

**1 ESP32 = 2 node_ids virtuais**

- **Sensor 1 (CIE1)**: `TRIG=GPIO1`, `ECHO=GPIO0` → `node_id=4`
- **Sensor 2 (CIE2)**: `TRIG=GPIO3`, `ECHO=GPIO2` → `node_id=5`

## 🔧 Características

✅ **Leitura independente** - Cada sensor mede e envia separadamente  
✅ **Filtro Kalman por sensor** - Estado persistente entre leituras  
✅ **Detecção de anomalias individual** - Baseline separado para cada reservatório  
✅ **Tratamento de erro robusto** - Um sensor pode falhar, o outro continua operando  
✅ **Sequências NVS separadas** - `seq1` e `seq2` para CIE1 e CIE2  
✅ **Redundância de gateway** - Até 3 gateways com failover  
✅ **ACK protocol** - Confirmação de entrega para ambos os pacotes  

## 📦 Hardware

- **ESP32-C3 Supermini**
- **2x HC-SR04** ultrasonic sensors
- **Voltage divider** on GPIO4 (ADC1_CH4) para monitorar Vin
- **LED** on GPIO8

### Pinagem

| Função | GPIO | Descrição |
|--------|------|-----------|
| TRIG_1 | GPIO1 | Trigger sensor 1 (CIE1) |
| ECHO_1 | GPIO0 | Echo sensor 1 (CIE1) |
| TRIG_2 | GPIO3 | Trigger sensor 2 (CIE2) |
| ECHO_2 | GPIO2 | Echo sensor 2 (CIE2) |
| ADC_VIN | GPIO4 | Leitura de tensão (voltage divider) |
| LED | GPIO8 | LED embutido |

## 🚀 Build & Flash

```bash
cd ~/firmware_aguada/firmware/node_cie_dual

# Configure target
idf.py set-target esp32c3

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor

# Monitor without reset
idf.py -p /dev/ttyUSB0 monitor --no-reset
```

## 📊 Fluxo de Operação

```
Loop (a cada 30s):
  1. Medir SENSOR 1 (GPIO1/0)
     ├─ 3 leituras com Kalman filter
     ├─ Calcular level/volume
     ├─ Detectar anomalias
     └─ Enviar pacote com node_id=4
  
  2. Aguardar 100ms (evitar interferência GPIO)
  
  3. Medir SENSOR 2 (GPIO3/2)
     ├─ 3 leituras com Kalman filter
     ├─ Calcular level/volume
     ├─ Detectar anomalias
     └─ Enviar pacote com node_id=5
  
  4. Aguardar 30s até próximo ciclo
```

## 🔍 Logs Exemplo

```
I (1234) node_cie_dual: ═══════════════════════════════════════════
I (1235) node_cie_dual: 📊 CIE1 (node_id=4) - Medição #42
I (1236) node_cie_dual: ═══════════════════════════════════════════
I (1240) node_cie_dual:   Tentativa 1/3: raw=123cm, kalman=123cm ✓
I (1300) node_cie_dual:   Tentativa 2/3: raw=125cm, kalman=124cm ✓
I (1360) node_cie_dual:   Tentativa 3/3: raw=122cm, kalman=123cm ✓
I (1361) node_cie_dual: ✅ CIE1: Distância final (Kalman): 123cm (3/3 leituras válidas)
I (1362) node_cie_dual: CIE1: distance=123cm → level=327cm, 72%, 58400L, vin=5000mV
I (1365) node_cie_dual: 📤 Enviando para Gateway 0 (tentativa 1) node_id=4 seq=42
I (1550) node_cie_dual: ✅ ACK recebido: node_id=4, seq=42, status=0, rssi=-45, gw=0
I (1551) node_cie_dual: ✅ ACK confirmado! Taxa de sucesso: 98.5% (67/68)
I (1552) node_cie_dual: ✅ CIE1: Pacote enviado com sucesso (seq=42)

I (1652) node_cie_dual: ═══════════════════════════════════════════
I (1653) node_cie_dual: 📊 CIE2 (node_id=5) - Medição #38
I (1654) node_cie_dual: ═══════════════════════════════════════════
I (1658) node_cie_dual:   Tentativa 1/3: raw=200cm, kalman=200cm ✓
...
```

## ⚠️ Tratamento de Erros

### Sensor Individual Falha

Se um sensor falha completamente (3 tentativas inválidas):
- Envia pacote com `distance_cm=-1`
- `level_cm=0`, `percentual=0`, `volume_l=0`
- `flags=FLAG_IS_ALERT`, `alert_type=ALERT_SENSOR_STUCK`
- **O outro sensor continua operando normalmente**

### Gateway Offline

- Retry com exponential backoff
- Tenta até 3 gateways
- Taxa de sucesso registrada: `successful_acks / total_attempts`
- Último gateway bem-sucedido salvo em NVS

## 🔧 Configuração

### Alterar Node IDs

```cpp
// Em node_cie_dual.cpp
#define NODE_ID_1  4  // CIE1
#define NODE_ID_2  5  // CIE2
```

### Alterar Geometria do Reservatório

```cpp
// Ambos os reservatórios usam mesma geometria
#define VOL_MAX_L         80000   // 80.000 litros
#define LEVEL_MAX_CM      450     // 450 cm altura máxima
#define SENSOR_OFFSET_CM  20      // 20 cm acima do nível máximo
```

### Adicionar Mais Gateways

```cpp
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1
    {0x24, 0x0a, 0xc4, 0x9a, 0x58, 0x28},  // Gateway 2 (exemplo)
    {0x48, 0x3f, 0xda, 0x4a, 0x3c, 0x90}   // Gateway 3 (exemplo)
};
```

## 📝 Backend Integration

O backend deve:
1. Reconhecer `node_id=4` como "CIE1 - Cisterna Ilha Engenho 01"
2. Reconhecer `node_id=5` como "CIE2 - Cisterna Ilha Engenho 02"
3. Processar ambos os pacotes independentemente
4. Dashboard pode mostrar ambos reservatórios lado a lado

### Configuração SQL Completa

📄 **Ver arquivo completo**: `node_cie_dual/backend_config.sql`

```bash
# Executar configuração do banco de dados
mysql -u usuario -p sensores_db < node_cie_dual/backend_config.sql
```

### Exemplo SQL Rápido

```sql
-- Configuração dos 2 reservatórios da Cisterna Ilha do Engenho
-- Nota: node_id=4 usa MAC real do ESP32, node_id=5 usa MAC fictício para diferenciação no backend
INSERT INTO node_configs (node_id, mac, location, sensor_offset_cm, level_max_cm, vol_max_l) VALUES
(4, 'C8:2B:96:XX:XX:XX', 'CIE1 - Cisterna Ilha Engenho 01', 20, 450, 245000),
(5, 'AA:BB:CC:DD:EE:01', 'CIE2 - Cisterna Ilha Engenho 02', 20, 450, 245000);
```

**Estratégia de MACs**:
- `node_id=4` (CIE1): Usa **MAC real** do ESP32 físico (extrair via `idf.py monitor`)
- `node_id=5` (CIE2): Usa **MAC fictício** `AA:BB:CC:DD:EE:01` para diferenciação no backend

**Capacidade**: 245.000 litros por reservatório (Cisterna Ilha do Engenho)

**Por que MACs diferentes?**
- Backend pode filtrar/agrupar por MAC
- Dashboard pode identificar visualmente qual sensor
- Logs e métricas separadas por MAC
- Facilita troubleshooting (saber qual sensor físico tem problema)

### Extrair MAC Real do ESP32

```bash
cd ~/firmware_aguada/firmware/node_cie_dual
idf.py -p /dev/ttyUSB0 monitor --no-reset
```

Procurar no log:
```
I (1234) node_cie_dual: Device MAC: C8:2B:96:AA:BB:CC
```

Copiar `C8:2B:96:AA:BB:CC` e substituir no SQL do `node_id=4`.

## 🎯 Casos de Uso

### Cisterna com 2 Compartimentos

- **CIE**: 2 reservatórios lado a lado, parede divisória
- **1 ESP32** no topo da cisterna
- **2 sensores** apontados para baixo, um em cada compartimento
- **Dashboard** mostra nível de cada lado

### Monitoramento Diferenciado

- Reservatório 1: água potável
- Reservatório 2: água de reuso
- Alertas independentes por reservatório

## 🔄 Migração de node_ultra1

Se você já tem `node_ultra1` funcionando e quer migrar para dual-sensor:

1. **Copiar configuração**:
```bash
cp node_ultra1/sdkconfig.defaults node_cie_dual/
```

2. **Flashar novo firmware**:
```bash
cd node_cie_dual
idf.py flash monitor
```

3. **Atualizar backend** para reconhecer `node_id=4` e `node_id=5`

## 📚 Referências

- Baseado em `node_ultra1` (single sensor)
- Usa mesmos componentes: `ultrasonic01.h`, `level_calculator.h`
- Protocolo: `SensorPacketV1` com `node_id` diferente por sensor
- ACK protocol: mesma implementação de `node_ultra1`
