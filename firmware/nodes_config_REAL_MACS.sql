-- ============================================================================
-- CONFIGURAÇÃO DOS NODES COM MACs REAIS EXTRAÍDOS
-- Data: 17/12/2025
-- Sistema: Aguada Water Level Telemetry (730.000L total capacity)
-- ============================================================================

USE sensores_db;

-- ============================================================================
-- NODES PADRÃO (1-3): Reservatórios 80.000L
-- ============================================================================

-- Node 1: RCON - Castelo de Consumo
INSERT INTO node_configs (node_id, name, description, mac_address, 
                         sensor_type, sensor_offset_cm, level_max_cm, 
                         vol_max_l, location, active, installed_at)
VALUES (
    1,
    'RCON - Castelo de Consumo',
    'Reservatório elevado de agua potável para Consumo (80.000L)',
    '20:6E:F1:6B:77:58',  -- MAC REAL extraído
    'AJ-SR04M',
    20,   -- Sensor 20cm acima do nível máximo
    450,  -- Altura máxima de água
    80000, -- Capacidade total
    'Castelo de Consumo',
    1,
    NOW()
)
ON DUPLICATE KEY UPDATE
    mac_address = '20:6E:F1:6B:77:58',
    description = 'Reservatório de Consumo (80.000L)',
    updated_at = NOW();

-- Node 2: RCAV - Reservatório Água Combate a Incêndio
INSERT INTO node_configs (node_id, name, description, mac_address, 
                         sensor_type, sensor_offset_cm, level_max_cm, 
                         vol_max_l, location, active, installed_at)
VALUES (
    2,
    'RCAV - Castelo de Incêndio',
    'Reservatório Rede Combate a Incêndio (80.000L)',
    'DC:06:75:67:6A:CC',  -- MAC REAL extraído
    'AJ-SR04M',
    20,
    450,
    80000,
    'Castelo de Incêndio',
    1,
    NOW()
)
ON DUPLICATE KEY UPDATE
    mac_address = 'DC:06:75:67:6A:CC',
    description = 'Reservatório Rede Combate a Incêndio (80.000L)',
    updated_at = NOW();

-- Node 3: RCB3 - Reservatório Casa de Bombas 03
INSERT INTO node_configs (node_id, name, description, mac_address, 
                         sensor_type, sensor_offset_cm, level_max_cm, 
                         vol_max_l, location, active, installed_at)
VALUES (
    3,
    'RCB3 - Casa de Bombas 03',
    'Reservatório Casa de Bombas 03 (80.000L)',
    '80:F1:B2:50:31:34',  -- MAC REAL extraído
    'HC-SR04',
    20,
    450,
    80000,
    'Casa de Bombas 03',
    1,
    NOW()
)
ON DUPLICATE KEY UPDATE
    mac_address = '80:F1:B2:50:31:34',
    description = 'Reservatório Casa de Bombas 03 (80.000L)',
    updated_at = NOW();

-- ============================================================================
-- NODES CIE (4-5): Reservatórios 245.000L cada (DUAL SENSOR)
-- ============================================================================

-- Node 4: CIE1 - Reservatório CIE Inferior (245.000L)
INSERT INTO node_configs (node_id, name, description, mac_address, 
                         sensor_type, sensor_offset_cm, level_max_cm, 
                         vol_max_l, location, active, installed_at)
VALUES (
    4,
    'CIE1 - Cisternas Ilha do Engenho 01',
    'Cisterna Ilha do Engenho N1 (245.000L)',
    'DC:B4:D9:8B:9E:AC',  -- MAC REAL extraído (físico node_cie_dual)
    'HC-SR04',
    20,
    500,  -- Altura máxima: 5 metros
    245000, -- Capacidade total
    'CIE - Cisternas Ilha do Engenho',
    1,
    NOW()
)
ON DUPLICATE KEY UPDATE
    mac_address = 'DC:B4:D9:8B:9E:AC',
    description = 'Cisterna Ilha do Engenho N1 (245.000L)',
    level_max_cm = 500,
    vol_max_l = 245000,
    updated_at = NOW();

-- Node 5: CIE2 - Reservatório CIE Superior (245.000L)
INSERT INTO node_configs (node_id, name, description, mac_address, 
                         sensor_type, sensor_offset_cm, level_max_cm, 
                         vol_max_l, location, active, installed_at)
VALUES (
    5,
    'CIE2 - Reservatório Superior',
    'Centro de Informações Estratégicas - Reservatório Superior (245.000L)',
    'DC:B4:D9:8B:9E:AC',  -- MESMO MAC (dual sensor, NODE_ID diferente)
    'HC-SR04',
    20,
    500,  -- Altura máxima: 12 metros
    245000, -- Capacidade total
    'CIE - Cisternas Ilha do Engenho',
    1,
    NOW()
)
ON DUPLICATE KEY UPDATE
    mac_address = 'DC:B4:D9:8B:9E:AC',
    description = 'Centro de Informações Estratégicas - Reservatório Superior (245.000L)',
    level_max_cm = 1200,
    vol_max_l = 245000,
    updated_at = NOW();

-- ============================================================================
-- GATEWAY CONFIGURATION
-- ============================================================================

-- Gateway MAC: 80:F3:DA:62:A7:84 (ESP32 DevKit V1)
-- Canal: 11
-- IP: 192.168.0.130

-- ============================================================================
-- VERIFICAÇÃO
-- ============================================================================

SELECT 
    node_id,
    name,
    mac_address,
    vol_max_l / 1000 AS capacity_m3,
    location,
    active
FROM node_configs
ORDER BY node_id;

-- ============================================================================
-- RESUMO DO SISTEMA
-- ============================================================================
-- Total de nodes: 5
-- Total de sensores físicos: 4 ESP32-C3 + 5 HC-SR04
-- Capacidade total: 730.000 litros (730 m³)
--   - Nodes 1-3: 3 × 80.000L = 240.000L (240 m³)
--   - Nodes 4-5: 2 × 245.000L = 490.000L (490 m³)
-- 
-- Gateway: ESP32 DevKit V1 (80:F3:DA:62:A7:84)
-- Protocolo: ESP-NOW + HTTP POST
-- Backend: MySQL + PHP (porta 8080)
-- ============================================================================
