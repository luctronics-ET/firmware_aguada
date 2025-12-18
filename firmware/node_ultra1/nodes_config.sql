-- ============================================================================
-- Configuração de Nodes - Sistema Aguada IIoT
-- ============================================================================
-- Nodes com 1 sensor HC-SR04 cada (firmware: node_ultra1)
-- Reservatórios padrão: 80.000L, 450cm altura, 20cm offset
-- ============================================================================

-- Criar tabela se não existir
CREATE TABLE IF NOT EXISTS node_configs (
    node_id INT PRIMARY KEY,
    mac VARCHAR(17) NOT NULL UNIQUE,
    location VARCHAR(255) NOT NULL,
    sensor_offset_cm INT NOT NULL DEFAULT 20,
    level_max_cm INT NOT NULL DEFAULT 450,
    vol_max_l INT NOT NULL DEFAULT 80000,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    notes TEXT,
    INDEX idx_mac (mac),
    INDEX idx_location (location)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- NODE 1 - RCON (Castelo de Consumo)
-- ============================================================================
-- MAC: Substituir pelos 6 últimos dígitos do MAC real após flash
-- Localização: Reservatório de Consumo no Castelo
-- Capacidade: 80.000 litros

INSERT INTO node_configs (
    node_id,
    mac,
    location,
    sensor_offset_cm,
    level_max_cm,
    vol_max_l,
    notes
) VALUES (
    1,
    'C8:2B:96:XX:XX:XX',  -- ⚠️ SUBSTITUIR pelo MAC real do ESP32 #1
    'RCON - Castelo de Consumo',
    20,
    450,
    80000,
    'Reservatório principal de consumo do Castelo. Sensor: HC-SR04. Firmware: node_ultra1. Abastece demanda diária.'
) ON DUPLICATE KEY UPDATE
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l),
    notes = VALUES(notes),
    updated_at = CURRENT_TIMESTAMP;

-- ============================================================================
-- NODE 2 - RCAV (Castelo de Incêndio)
-- ============================================================================
-- MAC: Substituir pelos 6 últimos dígitos do MAC real após flash
-- Localização: Reservatório de Incêndio no Castelo
-- Capacidade: 80.000 litros

INSERT INTO node_configs (
    node_id,
    mac,
    location,
    sensor_offset_cm,
    level_max_cm,
    vol_max_l,
    notes
) VALUES (
    2,
    'C8:2B:96:YY:YY:YY',  -- ⚠️ SUBSTITUIR pelo MAC real do ESP32 #2
    'RCAV - Castelo de Incêndio',
    20,
    450,
    80000,
    'Reservatório de segurança contra incêndio do Castelo. Sensor: HC-SR04. Firmware: node_ultra1. Nível crítico: manter sempre acima de 70%.'
) ON DUPLICATE KEY UPDATE
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l),
    notes = VALUES(notes),
    updated_at = CURRENT_TIMESTAMP;

-- ============================================================================
-- NODE 3 - RCB3
-- ============================================================================
-- MAC: Substituir pelos 6 últimos dígitos do MAC real após flash
-- Localização: Reservatório RCB3
-- Capacidade: 80.000 litros

INSERT INTO node_configs (
    node_id,
    mac,
    location,
    sensor_offset_cm,
    level_max_cm,
    vol_max_l,
    notes
) VALUES (
    3,
    'C8:2B:96:ZZ:ZZ:ZZ',  -- ⚠️ SUBSTITUIR pelo MAC real do ESP32 #3
    'RCB3 - Reservatório Bloco 3',
    20,
    450,
    80000,
    'Reservatório do Bloco 3. Sensor: HC-SR04. Firmware: node_ultra1.'
) ON DUPLICATE KEY UPDATE
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l),
    notes = VALUES(notes),
    updated_at = CURRENT_TIMESTAMP;

-- ============================================================================
-- QUERIES ÚTEIS - Monitoramento dos 3 Nodes
-- ============================================================================

-- Ver configuração de todos os nodes padrão (1, 2, 3)
SELECT 
    node_id,
    mac,
    location,
    CONCAT(vol_max_l/1000, 'm³') AS capacidade,
    CONCAT(level_max_cm, 'cm') AS altura_maxima,
    CONCAT(sensor_offset_cm, 'cm') AS offset
FROM node_configs 
WHERE node_id IN (1, 2, 3)
ORDER BY node_id;

-- ============================================================================
-- Últimas leituras de cada node
-- ============================================================================
SELECT 
    nc.node_id,
    nc.location,
    tp.level_cm,
    tp.percentual,
    ROUND(tp.volume_l/1000, 1) AS volume_m3,
    nc.vol_max_l/1000 AS capacidade_m3,
    FROM_UNIXTIME(tp.ts_ms/1000) AS ultima_leitura,
    TIMESTAMPDIFF(MINUTE, FROM_UNIXTIME(tp.ts_ms/1000), NOW()) AS minutos_atras,
    tp.rssi
FROM node_configs nc
LEFT JOIN telemetry_processed tp ON nc.node_id = tp.node_id
WHERE nc.node_id IN (1, 2, 3)
    AND tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = nc.node_id)
ORDER BY nc.node_id;

-- ============================================================================
-- Status de todos os reservatórios (incluindo CIE)
-- ============================================================================
SELECT 
    nc.node_id,
    nc.location,
    ROUND(tp.volume_l/1000, 1) AS volume_atual_m3,
    nc.vol_max_l/1000 AS capacidade_m3,
    tp.percentual AS percentual,
    CASE 
        WHEN tp.percentual >= 70 THEN '🟢 Normal'
        WHEN tp.percentual >= 50 THEN '🟡 Atenção'
        WHEN tp.percentual >= 30 THEN '🟠 Baixo'
        ELSE '🔴 Crítico'
    END AS status,
    CASE 
        WHEN TIMESTAMPDIFF(MINUTE, FROM_UNIXTIME(tp.ts_ms/1000), NOW()) < 5 THEN '🟢 Online'
        WHEN TIMESTAMPDIFF(MINUTE, FROM_UNIXTIME(tp.ts_ms/1000), NOW()) < 15 THEN '🟡 Warning'
        ELSE '⚫ Offline'
    END AS conectividade,
    FROM_UNIXTIME(tp.ts_ms/1000) AS ultima_leitura
FROM node_configs nc
LEFT JOIN telemetry_processed tp ON nc.node_id = tp.node_id
WHERE tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = nc.node_id)
ORDER BY nc.node_id;

-- ============================================================================
-- Volume total disponível (todos os reservatórios)
-- ============================================================================
SELECT 
    COUNT(DISTINCT nc.node_id) AS total_reservatorios,
    SUM(nc.vol_max_l)/1000 AS capacidade_total_m3,
    SUM(ROUND(tp.volume_l/1000, 1)) AS volume_atual_m3,
    ROUND(AVG(tp.percentual), 1) AS percentual_medio,
    ROUND((SUM(tp.volume_l) / SUM(nc.vol_max_l)) * 100, 1) AS percentual_total
FROM node_configs nc
LEFT JOIN telemetry_processed tp ON nc.node_id = tp.node_id
WHERE tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = nc.node_id);

-- ============================================================================
-- Comparação: Consumo vs Incêndio vs RCB3
-- ============================================================================
SELECT 
    MAX(CASE WHEN nc.node_id = 1 THEN tp.percentual END) AS rcon_consumo,
    MAX(CASE WHEN nc.node_id = 2 THEN tp.percentual END) AS rcav_incendio,
    MAX(CASE WHEN nc.node_id = 3 THEN tp.percentual END) AS rcb3,
    MAX(CASE WHEN nc.node_id = 1 THEN ROUND(tp.volume_l/1000, 1) END) AS vol_consumo_m3,
    MAX(CASE WHEN nc.node_id = 2 THEN ROUND(tp.volume_l/1000, 1) END) AS vol_incendio_m3,
    MAX(CASE WHEN nc.node_id = 3 THEN ROUND(tp.volume_l/1000, 1) END) AS vol_rcb3_m3
FROM node_configs nc
LEFT JOIN telemetry_processed tp ON nc.node_id = tp.node_id
WHERE nc.node_id IN (1, 2, 3)
    AND tp.ts_ms = (SELECT MAX(ts_ms) FROM telemetry_processed WHERE node_id = nc.node_id);

-- ============================================================================
-- Alertas recentes (últimas 24h) - todos os nodes
-- ============================================================================
SELECT 
    nc.location,
    CASE tp.alert_type
        WHEN 1 THEN '🚨 Vazamento Rápido'
        WHEN 2 THEN '⚠️  Inundação'
        WHEN 3 THEN '🔧 Sensor Travado'
        ELSE 'Desconhecido'
    END AS tipo_alerta,
    tp.level_cm,
    tp.percentual,
    FROM_UNIXTIME(tp.ts_ms/1000) AS quando,
    TIMESTAMPDIFF(HOUR, FROM_UNIXTIME(tp.ts_ms/1000), NOW()) AS horas_atras
FROM telemetry_processed tp
JOIN node_configs nc ON tp.node_id = nc.node_id
WHERE tp.flags & 1  -- Bit 0 = tem alerta
    AND tp.ts_ms >= UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 24 HOUR)) * 1000
ORDER BY tp.ts_ms DESC;

-- ============================================================================
-- Consumo diário (últimos 7 dias) - por reservatório
-- ============================================================================
SELECT 
    DATE(FROM_UNIXTIME(ts_ms/1000)) AS dia,
    nc.location,
    MIN(tp.percentual) AS min_percent,
    MAX(tp.percentual) AS max_percent,
    MAX(tp.percentual) - MIN(tp.percentual) AS variacao,
    COUNT(*) AS num_leituras
FROM telemetry_processed tp
JOIN node_configs nc ON tp.node_id = nc.node_id
WHERE nc.node_id IN (1, 2, 3)
    AND tp.ts_ms >= UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 7 DAY)) * 1000
GROUP BY dia, nc.location
ORDER BY dia DESC, nc.node_id;

-- ============================================================================
-- NOTAS IMPORTANTES
-- ============================================================================
-- 
-- 1. EXTRAÇÃO DE MAC:
--    Após flashear cada ESP32, executar:
--    idf.py -p /dev/ttyUSB0 monitor
--    
--    No boot, procurar linha:
--    I (xxx) node_ultra1: Device MAC: C8:2B:96:XX:XX:XX
--    
--    Anotar os 6 últimos dígitos e atualizar este arquivo.
--
-- 2. EXECUÇÃO DO SQL:
--    mysql -u root -p sensores_db < nodes_config.sql
--
-- 3. VERIFICAÇÃO:
--    mysql -u root -p sensores_db -e "SELECT * FROM node_configs WHERE node_id IN (1,2,3);"
--
-- 4. FIRMWARE:
--    Todos os 3 nodes usam: node_ultra1/main/node_ultra1.cpp
--    Único ajuste necessário: #define NODE_ID no firmware (1, 2 ou 3)
--
-- 5. CAPACIDADE TOTAL SISTEMA:
--    - RCON (node 1):  80.000L
--    - RCAV (node 2):  80.000L
--    - RCB3 (node 3):  80.000L
--    - CIE1 (node 4): 245.000L
--    - CIE2 (node 5): 245.000L
--    TOTAL:           730.000L (730m³)
--
-- ============================================================================
