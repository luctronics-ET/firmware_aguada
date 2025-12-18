# 🚀 node_cie_dual - Guia Rápido de Referência

## ⚡ Quick Start (3 minutos)

```bash
# 1. Compilar e flash
cd ~/firmware_aguada/firmware/node_cie_dual
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# 2. Anotar o MAC exibido no boot
# Exemplo: "Device MAC: C8:2B:96:XX:XX:XX"

# 3. Configurar backend
nano backend_config.sql  # Substituir XX:XX:XX pelo MAC real
mysql -u root -p sensores_db < backend_config.sql

# 4. Conectar sensores
# HC-SR04 #1: TRIG→GPIO1, ECHO→GPIO0
# HC-SR04 #2: TRIG→GPIO3, ECHO→GPIO2

# 5. Pronto! Monitorar logs
idf.py -p /dev/ttyUSB0 monitor --no-reset
```

---

## 📋 Comandos Essenciais

### Build & Flash
```bash
# Build simples
idf.py build

# Flash em porta específica
idf.py -p /dev/ttyUSB0 flash

# Flash + Monitor em um comando
idf.py -p /dev/ttyUSB0 flash monitor

# Full clean (quando algo dá errado)
idf.py fullclean && idf.py build
```

### Monitor Serial
```bash
# Monitor sem reset (mantém ESP rodando)
idf.py -p /dev/ttyUSB0 monitor --no-reset

# Monitor com filtros (apenas erros)
idf.py -p /dev/ttyUSB0 monitor | grep -E "(ERROR|WARN)"

# Salvar logs em arquivo
idf.py -p /dev/ttyUSB0 monitor > logs.txt 2>&1
```

### Configuração
```bash
# Menuconfig (alterar opções)
idf.py menuconfig

# Ver configuração atual
cat sdkconfig | grep CONFIG_LED

# Resetar para defaults
rm sdkconfig && idf.py reconfigure
```

### Backend SQL
```bash
# Executar configuração
mysql -u root -p sensores_db < backend_config.sql

# Verificar nodes cadastrados
mysql -u root -p sensores_db -e "SELECT * FROM node_configs WHERE node_id IN (4,5);"

# Ver últimas leituras
mysql -u root -p sensores_db -e "
  SELECT node_id, level_cm, percentual, volume_l, FROM_UNIXTIME(ts_ms/1000) 
  FROM telemetry_processed 
  WHERE node_id IN (4,5) 
  ORDER BY ts_ms DESC 
  LIMIT 10;
"

# Comparar níveis CIE1 vs CIE2
mysql -u root -p sensores_db -e "
  SELECT 
    MAX(CASE WHEN node_id=4 THEN level_cm END) AS cie1_level,
    MAX(CASE WHEN node_id=5 THEN level_cm END) AS cie2_level,
    ABS(MAX(CASE WHEN node_id=4 THEN level_cm END) - 
        MAX(CASE WHEN node_id=5 THEN level_cm END)) AS diferenca_cm
  FROM telemetry_processed 
  WHERE node_id IN (4,5) 
  GROUP BY DATE(FROM_UNIXTIME(ts_ms/1000)) 
  ORDER BY ts_ms DESC 
  LIMIT 7;
"
```

---

## 🔧 Pinout (Decorado)

```
ESP32-C3 Supermini
┌─────────────────┐
│      USB-C      │
├─────────────────┤
│ GND          5V │ ← Alimentação
│ GPIO0      GPIO1│ ← Sensores
│ GPIO2      GPIO3│
│ ...          ...│
│ GPIO8 (LED) GND │
└─────────────────┘

Sensor 1 (CIE1):               Sensor 2 (CIE2):
HC-SR04                        HC-SR04
┌──────┐                       ┌──────┐
│ VCC  │ → 5V                  │ VCC  │ → 5V
│ TRIG │ → GPIO1               │ TRIG │ → GPIO3
│ ECHO │ → GPIO0               │ ECHO │ → GPIO2
│ GND  │ → GND                 │ GND  │ → GND
└──────┘                       └──────┘
```

---

## 📊 Interpretação de Logs

### ✅ Operação Normal
```
I (12345) node_cie_dual: === Sensor 1 (node_id=4) ===
I (12350) node_cie_dual: Distance: 123cm -> Level: 327cm (72%), Vol: 176543L
I (12360) node_cie_dual: ACK received! (95.2% success rate)
I (12460) node_cie_dual: === Sensor 2 (node_id=5) ===
I (12465) node_cie_dual: Distance: 132cm -> Level: 318cm (68%), Vol: 167234L
I (12475) node_cie_dual: ACK received! (96.1% success rate)
```

### ⚠️ Sensor com Problemas
```
W (12345) node_cie_dual: === Sensor 1 (node_id=4) ===
E (12350) ultrasonic01: Distance measure failed (timeout)
W (12355) node_cie_dual: Measurement failed, skipping sensor 1
I (12460) node_cie_dual: === Sensor 2 (node_id=5) ===  ← Sensor 2 continua OK
```

### 🚨 Alerta de Anomalia
```
W (12345) node_cie_dual: *** ANOMALY DETECTED ***
W (12346) node_cie_dual: Alert type: 1 (rapid_drop)
W (12347) node_cie_dual: Delta: -65cm (vazamento?)
W (12348) node_cie_dual: ***********************
```

### 🔌 Gateway Offline
```
E (12345) node_cie_dual: ACK timeout for gateway 80:F3:DA:62:A7:84
I (12350) node_cie_dual: Trying next gateway...
I (12355) node_cie_dual: Switched to gateway 2 (AA:BB:CC:DD:EE:FF)
```

---

## 🐛 Troubleshooting Rápido

| Sintoma | Causa Provável | Solução Rápida |
|---------|----------------|----------------|
| `distance=-1` repetidamente | HC-SR04 desconectado | Verificar TRIG/ECHO/VCC/GND |
| `ACK timeout` em ambos sensores | Gateway offline | Verificar gateway rodando |
| Apenas 1 sensor funciona | GPIO mal conectado | Trocar cabos entre sensores |
| `alert_type=3` constante | Sensor travado | Limpar HC-SR04, verificar obstruções |
| Backend não recebe dados | SQL não configurado | Executar `backend_config.sql` |
| Volume sempre 0 no banco | `node_configs` vazio | Verificar `SELECT * FROM node_configs;` |

---

## 📈 Valores Esperados (CIE)

### Geometria dos Reservatórios
- **Capacidade**: 245.000 litros (245 m³) cada
- **Altura máxima**: 450 cm
- **Sensor offset**: 20 cm (topo)
- **Distância mínima**: 20 cm (tanque cheio)
- **Distância máxima**: 470 cm (tanque vazio)

### Cálculos
```
Level (cm) = (Sensor Offset + Altura Máxima) - Distance
Level = (20 + 450) - Distance = 470 - Distance

Percentual (%) = (Level / 450) × 100

Volume (L) = (Level / 450) × 245000
```

### Exemplos
| Distance | Level | Percentual | Volume (CIE1) | Volume (CIE2) | Total |
|----------|-------|------------|---------------|---------------|-------|
| 20 cm    | 450 cm| 100%       | 245.000 L     | 245.000 L     | 490.000 L |
| 123 cm   | 347 cm| 77%        | 189.000 L     | 189.000 L     | 378.000 L |
| 245 cm   | 225 cm| 50%        | 122.500 L     | 122.500 L     | 245.000 L |
| 367 cm   | 103 cm| 23%        | 56.350 L      | 56.350 L      | 112.700 L |
| 470 cm   | 0 cm  | 0%         | 0 L           | 0 L           | 0 L       |

---

## 🎯 Queries SQL Úteis

### Dashboard em Tempo Real
```sql
-- Níveis atuais
SELECT 
  nc.location,
  tp.level_cm,
  tp.percentual,
  ROUND(tp.volume_l/1000, 1) AS volume_m3,
  FROM_UNIXTIME(tp.ts_ms/1000) AS ultima_leitura,
  TIMESTAMPDIFF(MINUTE, FROM_UNIXTIME(tp.ts_ms/1000), NOW()) AS minutos_atras
FROM telemetry_processed tp
JOIN node_configs nc ON tp.node_id = nc.node_id
WHERE tp.node_id IN (4,5)
  AND tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = tp.node_id);
```

### Volume Total da Cisterna
```sql
SELECT 
  SUM(ROUND(tp.volume_l/1000, 1)) AS volume_total_m3,
  ROUND(AVG(tp.percentual), 1) AS percentual_medio,
  COUNT(DISTINCT tp.node_id) AS sensores_ativos
FROM telemetry_processed tp
WHERE tp.node_id IN (4,5)
  AND tp.ts_ms >= UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 5 MINUTE)) * 1000;
```

### Consumo Diário (In/Out)
```sql
SELECT 
  DATE(FROM_UNIXTIME(ts_ms/1000)) AS dia,
  node_id,
  ROUND(SUM(CASE WHEN volume_l > LAG(volume_l) OVER (PARTITION BY node_id ORDER BY ts_ms) 
            THEN (volume_l - LAG(volume_l) OVER (PARTITION BY node_id ORDER BY ts_ms))/1000 
            ELSE 0 END), 1) AS abastecimento_m3,
  ROUND(SUM(CASE WHEN volume_l < LAG(volume_l) OVER (PARTITION BY node_id ORDER BY ts_ms) 
            THEN (LAG(volume_l) OVER (PARTITION BY node_id ORDER BY ts_ms) - volume_l)/1000 
            ELSE 0 END), 1) AS consumo_m3
FROM telemetry_processed
WHERE node_id IN (4,5)
GROUP BY dia, node_id
ORDER BY dia DESC, node_id;
```

### Alertas Recentes
```sql
SELECT 
  nc.location,
  CASE tp.alert_type
    WHEN 1 THEN '🚨 Vazamento Rápido'
    WHEN 2 THEN '⚠️  Inundação'
    WHEN 3 THEN '🔧 Sensor Travado'
    ELSE 'Desconhecido'
  END AS tipo_alerta,
  tp.level_cm,
  FROM_UNIXTIME(tp.ts_ms/1000) AS quando
FROM telemetry_processed tp
JOIN node_configs nc ON tp.node_id = nc.node_id
WHERE tp.node_id IN (4,5)
  AND tp.flags & 1  -- Bit 0 = tem alerta
ORDER BY tp.ts_ms DESC
LIMIT 20;
```

---

## 📁 Arquivos de Referência

| Arquivo | Descrição | Quando Usar |
|---------|-----------|-------------|
| `README.md` | Visão geral e pinout | Primeira leitura |
| `IMPLEMENTATION_SUMMARY.md` | Resumo executivo completo | Deployment em produção |
| `DASHBOARD_DESIGN.md` | Design UI compacto | Implementar dashboard web |
| `backend_config.sql` | Configuração SQL | Setup inicial do backend |
| `deploy_cie_dual.sh` | Script interativo | Automação de tarefas |
| `node_cie_dual.cpp` | Código-fonte | Debug ou modificações |

---

## 🔗 Links Úteis

- **ESP-IDF Docs**: https://docs.espressif.com/projects/esp-idf/en/v6.1/
- **ESP-NOW Guide**: https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c3/api-reference/network/esp_now.html
- **HC-SR04 Datasheet**: https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf
- **Kalman Filter Intro**: https://www.kalmanfilter.net/default.aspx

---

## 🎓 Conceitos-Chave

### ESP-NOW
Protocolo Espressif para comunicação peer-to-peer sem router. Até 250 bytes por pacote, latência < 20ms.

### Filtro Kalman
Algoritmo que reduz ruído de sensores através de predição + correção. Exemplo: HC-SR04 varia ±2cm, Kalman reduz para ±0.3cm.

### NVS (Non-Volatile Storage)
Memória flash persistente no ESP32. Sobrevive a reboots. Usamos para: sequências, gateway preferido, configurações.

### Node ID Virtual
Um ESP32 físico simula 2 sensores independentes (node_id=4 e node_id=5). Backend trata como dispositivos separados.

### MAC Diferenciado
node_id=4 usa MAC real do ESP32. node_id=5 usa MAC fictício (AA:BB:CC:DD:EE:01). Facilita filtragem no backend.

---

## ⚡ Dicas de Performance

1. **RSSI < -70 dBm**: Gateway muito longe, considere repetidor
2. **Taxa ACK < 90%**: Interferência WiFi ou channel errado
3. **Heap < 50KB**: Considere aumentar `MALLOC_CAP_SPIRAM`
4. **Delay inter-sensor**: Mínimo 100ms, ideal 200ms para evitar interferência

---

## 🚀 Próximos Passos

Após deployment bem-sucedido:

1. [ ] Monitorar por 24h para validar estabilidade
2. [ ] Testar failover de gateway (desligar gateway principal)
3. [ ] Simular alertas (mover sensor rapidamente)
4. [ ] Implementar dashboard web (copiar de `DASHBOARD_DESIGN.md`)
5. [ ] Configurar backup automático do MySQL
6. [ ] Planejar expansão para outras cisternas

---

**Versão**: 1.0  
**Última atualização**: 17/12/2025  
**Licença**: MIT  
**Suporte**: Consultar `IMPLEMENTATION_SUMMARY.md` para troubleshooting detalhado
