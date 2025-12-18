-- ============================================================================
-- Configuração Backend para node_cie_dual
-- Cisterna Ilha do Engenho (CIE) - 2 Reservatórios Independentes
-- ============================================================================

-- Pré-requisito: Tabela node_configs deve existir
-- Se ainda não existe, criar com:

CREATE TABLE IF NOT EXISTS node_configs (
    node_id TINYINT UNSIGNED PRIMARY KEY,
    mac VARCHAR(17) NOT NULL,
    location VARCHAR(64) NOT NULL,
    
    -- Tank geometry
    sensor_offset_cm SMALLINT NOT NULL DEFAULT 20,
    level_max_cm SMALLINT NOT NULL DEFAULT 450,
    vol_max_l INT UNSIGNED NOT NULL DEFAULT 80000,
    
    -- Calibration
    distance_offset_cm SMALLINT NOT NULL DEFAULT 0,
    
    -- Anomaly detection thresholds
    rapid_change_threshold_cm SMALLINT NOT NULL DEFAULT 50,
    no_change_minutes SMALLINT UNSIGNED NOT NULL DEFAULT 120,
    
    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_mac (mac),
    INDEX idx_location (location)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Inserir configuração dos 2 sensores do node_cie_dual
-- ============================================================================

-- SENSOR 1: CIE1 - Reservatório 01 da Cisterna Ilha do Engenho
-- node_id=4, MAC real do ESP32 físico (substituir XX:XX:XX:XX:XX:XX pelo MAC real)
INSERT INTO node_configs (
    node_id, 
    mac, 
    location, 
    sensor_offset_cm, 
    level_max_cm, 
    vol_max_l,
    distance_offset_cm,
    rapid_change_threshold_cm,
    no_change_minutes
) VALUES (
    4,                                          -- node_id para CIE1
    'C8:2B:96:XX:XX:XX',                        -- MAC real do ESP32 (ALTERAR!)
    'CIE1 - Cisterna Ilha Engenho 01',         -- Localização
    20,                                         -- Sensor 20cm acima do nível máximo
    450,                                        -- Altura máxima: 450cm
    245000,                                     -- Capacidade: 245.000 litros
    0,                                          -- Sem offset de calibração inicial
    50,                                         -- Alerta se mudança >= 50cm
    120                                         -- Alerta se sem mudança por 120min
)
ON DUPLICATE KEY UPDATE
    mac = VALUES(mac),
    location = VALUES(location),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l),
    updated_at = CURRENT_TIMESTAMP;

-- SENSOR 2: CIE2 - Reservatório 02 da Cisterna Ilha do Engenho  
-- node_id=5, MAC fictício para diferenciação no backend
INSERT INTO node_configs (
    node_id,
    mac,
    location,
    sensor_offset_cm,
    level_max_cm,
    vol_max_l,
    distance_offset_cm,
    rapid_change_threshold_cm,
    no_change_minutes
) VALUES (
    5,                                          -- node_id para CIE2
    'AA:BB:CC:DD:EE:01',                        -- MAC fictício (diferencia de CIE1)
    'CIE2 - Cisterna Ilha Engenho 02',         -- Localização
    20,                                         -- Sensor 20cm acima do nível máximo
    450,                                        -- Altura máxima: 450cm
    245000,                                     -- Capacidade: 245.000 litros
    0,                                          -- Sem offset de calibração inicial
    50,                                         -- Alerta se mudança >= 50cm
    120                                         -- Alerta se sem mudança por 120min
)
ON DUPLICATE KEY UPDATE
    mac = VALUES(mac),
    location = VALUES(location),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l),
    updated_at = CURRENT_TIMESTAMP;

-- ============================================================================
-- Verificar configuração inserida
-- ============================================================================

SELECT 
    node_id,
    mac,
    location,
    CONCAT(vol_max_l / 1000, ' mil litros') AS capacidade,
    CONCAT(level_max_cm, ' cm') AS altura_maxima,
    created_at,
    updated_at
FROM node_configs
WHERE node_id IN (4, 5)
ORDER BY node_id;

-- ============================================================================
-- Consultas úteis
-- ============================================================================

-- Ver últimas leituras de ambos os sensores
SELECT 
    t.node_id,
    c.location,
    t.distance_cm,
    t.level_cm,
    t.percentual,
    CONCAT(ROUND(t.volume_l / 1000, 1), ' mil L') AS volume,
    t.rssi,
    FROM_UNIXTIME(t.ts_ms / 1000) AS timestamp,
    CASE 
        WHEN t.flags & 1 THEN '🚨 ALERTA'
        ELSE '✓ Normal'
    END AS status
FROM telemetry_processed t
JOIN node_configs c ON t.node_id = c.node_id
WHERE t.node_id IN (4, 5)
ORDER BY t.ts_ms DESC
LIMIT 10;

-- Comparar nível dos 2 reservatórios (última leitura)
SELECT 
    MAX(CASE WHEN node_id = 4 THEN level_cm END) AS cie1_level_cm,
    MAX(CASE WHEN node_id = 4 THEN percentual END) AS cie1_percentual,
    MAX(CASE WHEN node_id = 5 THEN level_cm END) AS cie2_level_cm,
    MAX(CASE WHEN node_id = 5 THEN percentual END) AS cie2_percentual,
    ABS(
        MAX(CASE WHEN node_id = 4 THEN level_cm END) - 
        MAX(CASE WHEN node_id = 5 THEN level_cm END)
    ) AS diferenca_cm
FROM (
    SELECT node_id, level_cm, percentual
    FROM telemetry_processed
    WHERE node_id IN (4, 5)
    ORDER BY ts_ms DESC
    LIMIT 2
) latest;

-- Histórico de alertas dos 2 reservatórios
SELECT 
    c.location,
    t.alert_type,
    CASE t.alert_type
        WHEN 1 THEN 'Queda Rápida (Vazamento)'
        WHEN 2 THEN 'Subida Rápida (Bomba/Inundação)'
        WHEN 3 THEN 'Sensor Travado'
        ELSE 'Desconhecido'
    END AS tipo_alerta,
    t.level_cm,
    FROM_UNIXTIME(t.ts_ms / 1000) AS timestamp
FROM telemetry_processed t
JOIN node_configs c ON t.node_id = c.node_id
WHERE t.node_id IN (4, 5)
  AND t.flags & 1  -- FLAG_IS_ALERT
ORDER BY t.ts_ms DESC
LIMIT 20;

-- Volume total dos 2 reservatórios (última leitura)
SELECT 
    SUM(volume_l) AS volume_total_litros,
    CONCAT(ROUND(SUM(volume_l) / 1000, 1), ' mil L') AS volume_total_formatado,
    ROUND(AVG(percentual), 1) AS percentual_medio
FROM (
    SELECT node_id, volume_l, percentual
    FROM telemetry_processed
    WHERE node_id IN (4, 5)
    ORDER BY ts_ms DESC
    LIMIT 2
) latest;

-- ============================================================================
-- NOTAS IMPORTANTES
-- ============================================================================

/*
1. EXTRAIR MAC REAL DO ESP32:
   cd ~/firmware_aguada/firmware/node_cie_dual
   idf.py -p /dev/ttyUSB0 monitor --no-reset
   
   Procurar linha no log:
   I (1234) node_cie_dual: Device MAC: C8:2B:96:XX:XX:XX
   
   Substituir 'C8:2B:96:XX:XX:XX' no INSERT do node_id=4

2. MAC FICTÍCIO node_id=5:
   - 'AA:BB:CC:DD:EE:01' é fictício, não precisa corresponder a hardware real
   - Serve apenas para diferenciar os 2 sensores no backend
   - Se adicionar mais ESP32 dual-sensor, use AA:BB:CC:DD:EE:02, etc.

3. CAPACIDADE DA CISTERNA:
   - Cada reservatório: 245.000 litros
   - Total CIE: 490.000 litros (245k × 2)
   - Altura: 450cm

4. BACKEND PHP:
   - Deve aceitar ambos node_id=4 e node_id=5
   - Processar independentemente (cálculos, anomalias)
   - Dashboard: mostrar lado a lado ou gráfico comparativo

5. ALERTAS:
   - Cada reservatório tem detecção de anomalia independente
   - Se CIE1 vazar, CIE2 continua operando normalmente
   - Alertas diferenciados por location
*/
