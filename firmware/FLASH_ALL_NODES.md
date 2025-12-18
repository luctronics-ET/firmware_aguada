# 🔥 Gravação Rápida - Todos os Nodes Aguada IIoT

**Objetivo**: Gravar firmware em **5 ESP32** sequencialmente e extrair MACs.

---

## 📦 Inventário de Hardware

| Node | ID | Nome | Local | Capacidade | Firmware | GPIO HC-SR04 |
|------|-----|------|-------|------------|----------|--------------|
| 1 | `node_id=1` | RCON | Castelo Consumo | 80.000L | `node_ultra1` | TRIG=GPIO1, ECHO=GPIO0 |
| 2 | `node_id=2` | RCAV | Castelo Incêndio | 80.000L | `node_ultra1` | TRIG=GPIO1, ECHO=GPIO0 |
| 3 | `node_id=3` | RCB3 | Casa de Bombas 03 | 80.000L | `node_ultra1` | TRIG=GPIO1, ECHO=GPIO0 |
| 4 | `node_id=4` | CIE1 | Cisterna Ilha Engenho 01 | 245.000L | `node_cie_dual` | Sensor1: TRIG=GPIO1, ECHO=GPIO0 |
| 5 | `node_id=5` | CIE2 | Cisterna Ilha Engenho 02 | 245.000L | `node_cie_dual` | Sensor2: TRIG=GPIO3, ECHO=GPIO2 |

**Hardware necessário**:
- 4× ESP32-C3 Supermini (nodes 1, 2, 3 + 1 para nodes 4/5)
- 5× HC-SR04 ultrassônicos
- 4× Cabos USB-C (para programação)
- 1× ESP32 DevKit V1 (gateway - já configurado)

---

## 🚀 Processo de Gravação (25 minutos total)

### **Preparação Inicial** (2 minutos)

```bash
cd ~/firmware_aguada/firmware

# Verificar ESP-IDF instalado
idf.py --version
# Esperado: ESP-IDF v6.1 ou superior

# Verificar portas USB disponíveis
ls /dev/ttyUSB* /dev/ttyACM*
```

---

## 📝 **ETAPA 1: Nodes Padrão (RCON, RCAV, RCB3)** (15 min)

Esses 3 nodes usam o **mesmo firmware** (`node_ultra1`), mas com **NODE_ID diferente**.

### **Node 1 - RCON (Castelo Consumo)**

```bash
cd ~/firmware_aguada/firmware/node_ultra1
```

**1.1. Editar NODE_ID:**
```bash
# Abrir arquivo de configuração
nano main/node_ultra1.cpp
```

Procure por `static const uint8_t NODE_ID` (linha ~110) e ajuste:
```cpp
static const uint8_t NODE_ID = 1;  // ← MUDAR PARA 1
```

Salvar: `Ctrl+O`, `Enter`, `Ctrl+X`

**1.2. Compilar e gravar:**
```bash
# Conectar ESP32-C3 #1 via USB
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**1.3. Extrair MAC ao bootar:**
Aguarde o boot e copie linha semelhante a:
```
I (500) node_ultra01: Device MAC: C8:2B:96:12:34:56
```

Anote: **Node 1 MAC = `C8:2B:96:12:34:56`** (exemplo)

**1.4. Desconectar ESP32-C3 #1**

---

### **Node 2 - RCAV (Castelo Incêndio)**

```bash
cd ~/firmware_aguada/firmware/node_ultra1
nano main/node_ultra1.cpp
```

Ajustar NODE_ID:
```cpp
static const uint8_t NODE_ID = 2;  // ← MUDAR PARA 2
```

Salvar e gravar:
```bash
# Conectar ESP32-C3 #2 via USB
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Extrair MAC ao bootar:
```
I (500) node_ultra01: Device MAC: C8:2B:96:AA:BB:CC
```

Anote: **Node 2 MAC = `C8:2B:96:AA:BB:CC`** (exemplo)

Desconectar ESP32-C3 #2.

---

### **Node 3 - RCB3 (Casa de Bombas 03)**

```bash
cd ~/firmware_aguada/firmware/node_ultra1
nano main/node_ultra1.cpp
```

Ajustar NODE_ID:
```cpp
static const uint8_t NODE_ID = 3;  // ← MUDAR PARA 3
```

Salvar e gravar:
```bash
# Conectar ESP32-C3 #3 via USB
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Extrair MAC ao bootar:
```
I (500) node_ultra01: Device MAC: C8:2B:96:DD:EE:FF
```

Anote: **Node 3 MAC = `C8:2B:96:DD:EE:FF`** (exemplo)

Desconectar ESP32-C3 #3.

---

## 📝 **ETAPA 2: Cisterna CIE (CIE1, CIE2)** (10 min)

O **node_cie_dual** controla **2 sensores** (nodes 4 e 5) com **1 único ESP32**.

```bash
cd ~/firmware_aguada/firmware/node_cie_dual
```

**2.1. Compilar e gravar:**
```bash
# Conectar ESP32-C3 #4 via USB
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**2.2. Extrair MAC ao bootar:**
```
I (500) node_cie_dual: Device MAC: C8:2B:96:77:88:99
```

Anote: **Node 4/5 (CIE) MAC = `C8:2B:96:77:88:99`** (exemplo)

**⚠️ IMPORTANTE**: Este ESP32 envia **2 pacotes** por ciclo:
- **Pacote 1**: `node_id=4` (sensor GPIO1/0)
- **Pacote 2**: `node_id=5` (sensor GPIO3/2)

Ambos usam o **mesmo MAC** (fictício para node_id=5 será `C8:2B:96:77:88:9A`).

Desconectar ESP32-C3 #4.

---

## 📊 **ETAPA 3: Atualizar SQL com MACs Reais** (3 min)

Abra os arquivos SQL e substitua os placeholders pelos MACs extraídos:

### **Nodes Padrão (1, 2, 3):**
```bash
nano ~/firmware_aguada/firmware/node_ultra1/nodes_config.sql
```

Substituir:
```sql
-- Node 1
('C8:2B:96:XX:XX:XX', 1, 'RCON - Castelo de Consumo', ...)
-- ↑ Substituir por: C8:2B:96:12:34:56

-- Node 2
('C8:2B:96:YY:YY:YY', 2, 'RCAV - Castelo de Incêndio', ...)
-- ↑ Substituir por: C8:2B:96:AA:BB:CC

-- Node 3
('C8:2B:96:ZZ:ZZ:ZZ', 3, 'RCB3 - Casa de Bombas 03', ...)
-- ↑ Substituir por: C8:2B:96:DD:EE:FF
```

### **Cisterna CIE (4, 5):**
```bash
nano ~/firmware_aguada/firmware/node_cie_dual/backend_config.sql
```

Substituir:
```sql
-- Node 4 (CIE1) - MAC real
('C8:2B:96:XX:XX:XX', 4, 'CIE1 - Cisterna Ilha Engenho 01', ...)
-- ↑ Substituir por: C8:2B:96:77:88:99

-- Node 5 (CIE2) - MAC fictício (last byte +1)
('C8:2B:96:XX:XX:XY', 5, 'CIE2 - Cisterna Ilha Engenho 02', ...)
-- ↑ Substituir por: C8:2B:96:77:88:9A
```

---

## 💾 **ETAPA 4: Executar SQL no Banco** (2 min)

```bash
# Nodes padrão (1, 2, 3)
mysql -u root -p sensores_db < ~/firmware_aguada/firmware/node_ultra1/nodes_config.sql

# Cisterna CIE (4, 5)
mysql -u root -p sensores_db < ~/firmware_aguada/firmware/node_cie_dual/backend_config.sql

# Verificar configuração
mysql -u root -p sensores_db -e "SELECT node_id, mac, location, vol_max_l FROM node_configs ORDER BY node_id;"
```

Saída esperada:
```
+---------+-------------------+----------------------------------+-----------+
| node_id | mac               | location                         | vol_max_l |
+---------+-------------------+----------------------------------+-----------+
|       1 | C8:2B:96:12:34:56 | RCON - Castelo de Consumo        |     80000 |
|       2 | C8:2B:96:AA:BB:CC | RCAV - Castelo de Incêndio       |     80000 |
|       3 | C8:2B:96:DD:EE:FF | RCB3 - Casa de Bombas 03         |     80000 |
|       4 | C8:2B:96:77:88:99 | CIE1 - Cisterna Ilha Engenho 01  |    245000 |
|       5 | C8:2B:96:77:88:9A | CIE2 - Cisterna Ilha Engenho 02  |    245000 |
+---------+-------------------+----------------------------------+-----------+
```

---

## 🔌 **ETAPA 5: Instalação Física** (tempo variável)

### **Nodes Padrão (RCON, RCAV, RCB3):**
1. Instalar ESP32-C3 em caixa estanque
2. Conectar HC-SR04:
   - VCC → 5V
   - GND → GND
   - TRIG → GPIO1
   - ECHO → GPIO0
3. Alimentar ESP32 com fonte 5V/1A
4. Posicionar sensor no topo do reservatório (20cm acima do nível máximo)

### **Cisterna CIE (1 ESP32, 2 sensores):**
1. Instalar ESP32-C3 em posição central entre os dois reservatórios
2. Conectar HC-SR04 #1 (CIE1):
   - TRIG → GPIO1
   - ECHO → GPIO0
3. Conectar HC-SR04 #2 (CIE2):
   - TRIG → GPIO3
   - ECHO → GPIO2
4. Usar cabos longos se necessário (até 5 metros OK)
5. Alimentar ESP32 com fonte 5V/2A (mais corrente devido a 2 sensores)

---

## ✅ **ETAPA 6: Validação Final** (5 min)

### **Verificar gateway recebendo dados:**
```bash
cd ~/firmware_aguada/firmware/gateway_devkit_v1
idf.py -p /dev/ttyACM0 monitor --no-reset
```

Aguarde até 30 segundos. Você deve ver:
```
I (12345) gateway: Recebido de node_id=1, RSSI=-45, seq=1
I (12346) gateway: HTTP POST OK: 200
I (12375) gateway: Recebido de node_id=2, RSSI=-52, seq=1
I (12376) gateway: HTTP POST OK: 200
I (12405) gateway: Recebido de node_id=3, RSSI=-48, seq=1
I (12406) gateway: HTTP POST OK: 200
I (12435) gateway: Recebido de node_id=4, RSSI=-50, seq=1
I (12436) gateway: HTTP POST OK: 200
I (12465) gateway: Recebido de node_id=5, RSSI=-50, seq=2
I (12466) gateway: HTTP POST OK: 200
```

### **Verificar backend:**
```bash
mysql -u root -p sensores_db -e "
SELECT node_id, location, distance_cm, level_cm, percentual, 
       FROM_UNIXTIME(ts_ms/1000) AS timestamp
FROM telemetry_processed 
ORDER BY ts_ms DESC LIMIT 10;
"
```

Saída esperada (últimas 10 leituras de todos os nodes):
```
+---------+----------------------------------+-------------+----------+------------+---------------------+
| node_id | location                         | distance_cm | level_cm | percentual | timestamp           |
+---------+----------------------------------+-------------+----------+------------+---------------------+
|       5 | CIE2 - Cisterna Ilha Engenho 02  |         120 |      350 |         78 | 2025-12-17 14:32:15 |
|       4 | CIE1 - Cisterna Ilha Engenho 01  |         115 |      355 |         79 | 2025-12-17 14:32:14 |
|       3 | RCB3 - Casa de Bombas 03         |         180 |      290 |         64 | 2025-12-17 14:32:05 |
|       2 | RCAV - Castelo de Incêndio       |         150 |      320 |         71 | 2025-12-17 14:31:35 |
|       1 | RCON - Castelo de Consumo        |         200 |      270 |         60 | 2025-12-17 14:31:05 |
+---------+----------------------------------+-------------+----------+------------+---------------------+
```

---

## 🎯 Checklist Final

- [ ] **Node 1 (RCON)**: Firmware gravado, MAC extraído, sensor conectado, dados chegando
- [ ] **Node 2 (RCAV)**: Firmware gravado, MAC extraído, sensor conectado, dados chegando
- [ ] **Node 3 (RCB3)**: Firmware gravado, MAC extraído, sensor conectado, dados chegando
- [ ] **Node 4/5 (CIE)**: Firmware gravado, MAC extraído, 2 sensores conectados, 2 pacotes/ciclo chegando
- [ ] **Backend SQL**: 5 linhas em `node_configs`, dados em `telemetry_processed`
- [ ] **Gateway**: Logs mostrando recepção de todos os 5 nodes
- [ ] **Dashboard**: (se implementado) Exibindo 5 cards com níveis em tempo real

---

## 🚨 Troubleshooting Comum

### **"ESP32 não reconhecido em /dev/ttyUSB0"**
```bash
# Verificar se driver CH340 instalado
lsusb | grep -i "CH340\|CP210\|FTDI"

# Adicionar usuário ao grupo dialout
sudo usermod -a -G dialout $USER
# Logout/login necessário
```

### **"idf.py: command not found"**
```bash
# Instalar ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Adicionar ao ~/.bashrc para permanente
echo '. $HOME/esp/esp-idf/export.sh' >> ~/.bashrc
```

### **"Gateway não recebe pacotes"**
1. Verificar canal 11 configurado em todos os firmwares
2. Verificar gateway MAC address no `node_ultra1.cpp` (linha ~82)
3. Testar distância (ESP-NOW funciona até ~100m em campo aberto)
4. Verificar alimentação 5V estável

### **"Sensor retorna sempre -1 (erro)"**
1. Verificar conexões: VCC=5V, GND=GND, TRIG=GPIO1, ECHO=GPIO0
2. Medir distância manualmente (deve estar entre 5cm e 450cm)
3. Trocar HC-SR04 (pode estar queimado)

### **"Backend não salva dados"**
```bash
# Verificar serviços rodando
~/firmware_aguada/firmware/status_services.sh

# Se não:
~/firmware_aguada/firmware/start_services.sh

# Testar endpoint manualmente
curl -X POST http://192.168.0.117:8080/ingest_sensorpacket.php \
  -H "Content-Type: application/json" \
  -d '{"node_id":1,"distance_cm":100}'
```

---

## 📞 Suporte

- **Documentação completa**: `~/firmware_aguada/firmware/README.md`
- **Guia node padrão**: `node_ultra1/NODES_SETUP_GUIDE.md`
- **Guia CIE dual**: `node_cie_dual/IMPLEMENTATION_SUMMARY.md`
- **Script interativo CIE**: `node_cie_dual/deploy_cie_dual.sh`

---

**Tempo total estimado: 25-30 minutos** (sem instalação física)

🎉 **Sistema pronto para produção!**
