# 📋 Configuração dos Nodes Padrão (RCON, RCAV, RCB3)

## 🎯 Visão Geral

Sistema com **3 reservatórios padrão** usando firmware `node_ultra1`:

| Node ID | Localização | Código | Capacidade | Função |
|---------|-------------|--------|------------|---------|
| 1 | Castelo | RCON | 80.000L | Reservatório de Consumo |
| 2 | Castelo | RCAV | 80.000L | Reservatório de Incêndio |
| 3 | Bloco 3 | RCB3 | 80.000L | Reservatório Geral |

**Especificações comuns**:
- Hardware: ESP32-C3 Supermini
- Sensor: 1× HC-SR04 por node
- Altura máxima: 450 cm
- Sensor offset: 20 cm
- Firmware: `node_ultra1/main/node_ultra1.cpp`

---

## 🚀 Processo de Configuração (Para cada node)

### Passo 1: Editar NODE_ID no Firmware

```bash
cd ~/firmware_aguada/firmware/node_ultra1
nano main/node_ultra1.cpp
```

Localizar e alterar a linha:
```cpp
#define NODE_ID 1  // Alterar para 1, 2 ou 3 conforme o node
```

### Passo 2: Compilar e Flash

```bash
# Compilar
idf.py build

# Flash (ajustar porta conforme necessário)
idf.py -p /dev/ttyUSB0 flash monitor
```

### Passo 3: Extrair MAC do ESP32

Durante o boot, no monitor serial procurar:
```
I (xxxx) node_ultra1: Device MAC: C8:2B:96:XX:XX:XX
```

**Anotar o MAC completo** (você precisará dele no próximo passo).

### Passo 4: Atualizar nodes_config.sql

```bash
nano node_ultra1/nodes_config.sql
```

Localizar as linhas:
```sql
-- Para node_id=1 (RCON):
mac = 'C8:2B:96:XX:XX:XX',  -- Substituir XX:XX:XX

-- Para node_id=2 (RCAV):
mac = 'C8:2B:96:YY:YY:YY',  -- Substituir YY:YY:YY

-- Para node_id=3 (RCB3):
mac = 'C8:2B:96:ZZ:ZZ:ZZ',  -- Substituir ZZ:ZZ:ZZ
```

### Passo 5: Executar SQL no Backend

```bash
mysql -u root -p sensores_db < node_ultra1/nodes_config.sql
```

### Passo 6: Verificar Configuração

```bash
mysql -u root -p sensores_db -e "SELECT * FROM node_configs WHERE node_id IN (1,2,3);"
```

Saída esperada:
```
+---------+-----------------+---------------------------+------------------+--------------+-----------+
| node_id | mac             | location                  | sensor_offset_cm | level_max_cm | vol_max_l |
+---------+-----------------+---------------------------+------------------+--------------+-----------+
|       1 | C8:2B:96:AA:BB:CC | RCON - Castelo de Consumo |               20 |          450 |     80000 |
|       2 | C8:2B:96:DD:EE:FF | RCAV - Castelo de Incêndio|               20 |          450 |     80000 |
|       3 | C8:2B:96:11:22:33 | RCB3 - Reservatório Bloco 3|              20 |          450 |     80000 |
+---------+-----------------+---------------------------+------------------+--------------+-----------+
```

### Passo 7: Conectar HC-SR04

```
HC-SR04 (para cada node):
┌──────┐
│ VCC  │ → 5V
│ TRIG │ → GPIO1
│ ECHO │ → GPIO0
│ GND  │ → GND
└──────┘
```

### Passo 8: Validar Operação

```bash
idf.py -p /dev/ttyUSB0 monitor --no-reset
```

Verificar logs:
- ✅ Sensor medindo distâncias válidas (20-470 cm)
- ✅ ACKs sendo recebidos do gateway
- ✅ Taxa de sucesso > 95%
- ✅ Volume calculado corretamente

---

## 📊 Checklist de Deployment

### Node 1 - RCON (Castelo de Consumo)
- [ ] NODE_ID definido como `1` no firmware
- [ ] Firmware compilado e flasheado
- [ ] MAC extraído: `C8:2B:96:__:__:__`
- [ ] SQL atualizado com MAC real
- [ ] Configuração verificada no MySQL
- [ ] HC-SR04 conectado (GPIO1/GPIO0)
- [ ] Sensor lendo corretamente
- [ ] Gateway recebendo pacotes

### Node 2 - RCAV (Castelo de Incêndio)
- [ ] NODE_ID definido como `2` no firmware
- [ ] Firmware compilado e flasheado
- [ ] MAC extraído: `C8:2B:96:__:__:__`
- [ ] SQL atualizado com MAC real
- [ ] Configuração verificada no MySQL
- [ ] HC-SR04 conectado (GPIO1/GPIO0)
- [ ] Sensor lendo corretamente
- [ ] Gateway recebendo pacotes

### Node 3 - RCB3 (Bloco 3)
- [ ] NODE_ID definido como `3` no firmware
- [ ] Firmware compilado e flasheado
- [ ] MAC extraído: `C8:2B:96:__:__:__`
- [ ] SQL atualizado com MAC real
- [ ] Configuração verificada no MySQL
- [ ] HC-SR04 conectado (GPIO1/GPIO0)
- [ ] Sensor lendo corretamente
- [ ] Gateway recebendo pacotes

---

## 🔧 Comandos Rápidos

### Flash de Múltiplos Nodes (Sequencial)

```bash
cd ~/firmware_aguada/firmware/node_ultra1

# Node 1 (RCON)
nano main/node_ultra1.cpp  # Alterar NODE_ID para 1
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
# [Anotar MAC]

# Node 2 (RCAV)
nano main/node_ultra1.cpp  # Alterar NODE_ID para 2
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
# [Anotar MAC]

# Node 3 (RCB3)
nano main/node_ultra1.cpp  # Alterar NODE_ID para 3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
# [Anotar MAC]
```

### Verificar Últimas Leituras

```sql
-- Ver status de todos os 3 nodes
SELECT 
    nc.node_id,
    nc.location,
    tp.percentual,
    ROUND(tp.volume_l/1000, 1) AS volume_m3,
    FROM_UNIXTIME(tp.ts_ms/1000) AS ultima_leitura
FROM node_configs nc
LEFT JOIN telemetry_processed tp ON nc.node_id = tp.node_id
WHERE nc.node_id IN (1, 2, 3)
    AND tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = nc.node_id);
```

### Monitorar em Tempo Real

```sql
-- Refresh a cada 5 segundos
WATCH -n 5 "mysql -u root -p sensores_db -e '
    SELECT node_id, location, percentual, 
           ROUND(volume_l/1000,1) AS vol_m3,
           FROM_UNIXTIME(ts_ms/1000) AS hora
    FROM node_configs nc
    LEFT JOIN telemetry_processed tp ON nc.node_id = tp.node_id
    WHERE nc.node_id IN (1,2,3)
        AND tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = nc.node_id);
'"
```

---

## 🎨 Dashboard - Exemplo de Layout

```
╔═══════════════════════════════════════════════════════════════════╗
║  🏰 Castelo - Reservatórios                                       ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  ┌──── RCON (Consumo) ─────┐    ┌──── RCAV (Incêndio) ──────┐   ║
║  │ ▓▓▓▓▓▓▓░░░ 68%          │    │ ▓▓▓▓▓▓▓▓▓░░ 85%           │   ║
║  │ 54m³ / 80m³             │    │ 68m³ / 80m³              │   ║
║  │ 🟢 14:23  -42dBm        │    │ 🟢 14:23  -45dBm         │   ║
║  └─────────────────────────┘    └──────────────────────────┘   ║
║                                                                   ║
║  ┌──── RCB3 (Bloco 3) ─────┐                                    ║
║  │ ▓▓▓▓░░░░░░░ 42%          │                                    ║
║  │ 34m³ / 80m³             │                                    ║
║  │ 🟢 14:22  -48dBm        │                                    ║
║  └─────────────────────────┘                                    ║
║                                                                   ║
║  📊 Total Castelo+RCB3: 156m³/240m³ (65%)                       ║
╚═══════════════════════════════════════════════════════════════════╝
```

---

## 📈 Capacidade Total do Sistema

Após configurar **todos os 5 nodes** (3 padrão + 2 CIE):

| Localização | Nodes | Capacidade | Total |
|-------------|-------|------------|-------|
| Castelo (Consumo/Incêndio) + RCB3 | 1, 2, 3 | 80.000L × 3 | 240.000L |
| Cisterna Ilha do Engenho (CIE) | 4, 5 | 245.000L × 2 | 490.000L |
| **TOTAL SISTEMA** | **5 nodes** | | **730.000L (730m³)** |

---

## 🐛 Troubleshooting

### Problema: NODE_ID incorreto após flash
**Sintoma**: Node envia pacotes, mas backend não reconhece  
**Solução**: Verificar `#define NODE_ID` no firmware, recompilar e re-flash

### Problema: MAC não aparece no boot
**Sintoma**: Monitor serial não mostra linha "Device MAC"  
**Solução**: Esperar boot completo (5-10 segundos), ou desconectar/reconectar USB

### Problema: Backend não calcula volumes
**Sintoma**: `volume_l` sempre NULL ou 0  
**Solução**: Verificar se `node_configs` tem entrada para aquele `node_id`

### Problema: Conflito de MAC
**Sintoma**: Erro "Duplicate entry for key 'mac'" ao executar SQL  
**Solução**: MACs devem ser únicos. Verificar se não reutilizou MAC de outro node

---

## 📝 Notas Importantes

1. **NODE_ID deve ser único**: Cada ESP32 precisa ter um NODE_ID diferente (1, 2, 3)
2. **MAC real vs fictício**: Diferente do node_cie_dual, aqui sempre use o MAC real do ESP32
3. **Ordem de deployment**: Pode configurar os 3 nodes em qualquer ordem
4. **Firmware idêntico**: Todos os 3 usam o mesmo firmware `node_ultra1`, apenas NODE_ID muda
5. **Gateway único**: Os 3 nodes enviam para o mesmo gateway configurado em `GATEWAY_MACS[]`

---

## 🔗 Referências

- **Firmware source**: `node_ultra1/main/node_ultra1.cpp`
- **Configuração SQL**: `node_ultra1/nodes_config.sql`
- **Copilot Instructions**: `.github/copilot-instructions.md`
- **Build system**: `node_ultra1/CMakeLists.txt`
- **Queries úteis**: Incluídas no `nodes_config.sql`

---

**Versão**: 1.0  
**Data**: 17/12/2025  
**Sistema**: Aguada IIoT  
**Total de Nodes**: 5 (3 padrão + 2 CIE dual-sensor)
