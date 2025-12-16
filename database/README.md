# Database - Aguada Telemetry

Schemas e migrations para o banco de dados MySQL.

## Setup

### 1. Criar Banco
```bash
mysql -u root -p
```

```sql
CREATE DATABASE aguada_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
GRANT ALL PRIVILEGES ON aguada_db.* TO 'aguada_user'@'localhost' IDENTIFIED BY 'senha_segura';
FLUSH PRIVILEGES;
EXIT;
```

### 2. Importar Schema
```bash
mysql -u root -p aguada_db < schema.sql
```

## Schema Atual

### Tabela: `leituras_v2`

Armazena telemetria raw dos sensores (SensorPacketV1).

| Campo | Tipo | Descrição |
|-------|------|-----------|
| id | INT AUTO_INCREMENT | PK |
| created_at | TIMESTAMP | Timestamp de inserção no DB |
| version | TINYINT | Versão do protocolo (1) |
| node_id | SMALLINT | ID do nó (1-5) |
| mac | VARCHAR(17) | MAC address do nó |
| seq | INT | Sequência monotônica |
| distance_cm | INT | Distância sensor→água (cm) |
| level_cm | INT | Nível calculado (cm) |
| percentual | TINYINT | Percentual 0-100 |
| volume_l | INT | Volume em litros |
| vin_mv | INT | Tensão de alimentação (mV) |
| rssi | TINYINT | Sinal ESP-NOW (dBm) |
| ts_ms | BIGINT | Timestamp do gateway (ms) |

**Índices:**
- `idx_leituras_v2_node_ts` em (node_id, created_at)

## Queries Úteis

### Verificar dados por nó
```sql
SELECT 
    node_id,
    COUNT(*) as total_leituras,
    MAX(created_at) as ultima_leitura,
    AVG(level_cm) as nivel_medio,
    MIN(rssi) as pior_sinal,
    MAX(rssi) as melhor_sinal
FROM leituras_v2 
GROUP BY node_id 
ORDER BY node_id;
```

### Últimas 24h por nó
```sql
SELECT node_id, COUNT(*) as leituras_24h
FROM leituras_v2 
WHERE created_at >= NOW() - INTERVAL 24 HOUR
GROUP BY node_id;
```

### Detectar falhas de transmissão (gaps > 2min)
```sql
SELECT 
    a.node_id,
    a.seq as seq_anterior,
    b.seq as seq_atual,
    b.seq - a.seq as gap_seq,
    TIMESTAMPDIFF(SECOND, a.created_at, b.created_at) as gap_segundos
FROM leituras_v2 a
JOIN leituras_v2 b ON a.node_id = b.node_id 
    AND b.id = (SELECT MIN(id) FROM leituras_v2 WHERE node_id = a.node_id AND id > a.id)
WHERE TIMESTAMPDIFF(SECOND, a.created_at, b.created_at) > 120
ORDER BY b.created_at DESC;
```

## Migrations (Futuro)

Estrutura para versionamento de schema:
```
migrations/
├── 001_initial_schema.sql
├── 002_add_alerts_table.sql
└── 003_add_users_table.sql
```

## Backup

```bash
# Backup completo
mysqldump -u root -p aguada_db > backup_$(date +%Y%m%d_%H%M%S).sql

# Backup apenas estrutura
mysqldump -u root -p --no-data aguada_db > schema_backup.sql

# Backup apenas dados
mysqldump -u root -p --no-create-info aguada_db > data_backup.sql
```
