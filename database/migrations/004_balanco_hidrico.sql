-- Migration 004: Tabelas de Balanço Hídrico e Relatórios de Serviço
-- Data: 2025-12-15
-- Propósito: Implementar cálculos de consumo/abastecimento e relatórios

-- ============================================================================
-- 1. TABELA DE EVENTOS DE ABASTECIMENTO
-- ============================================================================
CREATE TABLE IF NOT EXISTS eventos_abastecimento (
    id INT AUTO_INCREMENT PRIMARY KEY,
    datetime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    reservatorio_origem VARCHAR(20) NOT NULL,
    reservatorio_destino VARCHAR(20) NOT NULL,
    volume_litros INT NOT NULL COMMENT 'Volume transferido em litros',
    duracao_minutos INT COMMENT 'Duração do bombeamento em minutos',
    bomba_utilizada VARCHAR(50) COMMENT 'BOR_CB3_MD1, BOR_CB3_ME1, etc',
    vazao_lpm DECIMAL(10,2) COMMENT 'Vazão média em litros por minuto',
    operador VARCHAR(100),
    observacoes TEXT,
    INDEX idx_datetime (datetime),
    INDEX idx_destino (reservatorio_destino)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Registro de eventos de abastecimento entre reservatórios';

-- ============================================================================
-- 2. TABELA DE BALANÇO HÍDRICO (CALCULADO)
-- ============================================================================
CREATE TABLE IF NOT EXISTS balanco_hidrico (
    id INT AUTO_INCREMENT PRIMARY KEY,
    reservatorio_id VARCHAR(20) NOT NULL,
    periodo_inicio TIMESTAMP NOT NULL,
    periodo_fim TIMESTAMP NOT NULL,
    
    -- Volumes inicial e final (do sensor)
    volume_inicial_litros INT COMMENT 'Volume no início do período',
    volume_final_litros INT COMMENT 'Volume no fim do período',
    
    -- Entradas (abastecimento)
    entrada_total_litros INT DEFAULT 0 COMMENT 'Total de água recebida',
    entrada_eventos INT DEFAULT 0 COMMENT 'Número de eventos de entrada',
    
    -- Saídas (consumo calculado)
    consumo_litros INT COMMENT 'Consumo calculado (se balanço negativo)',
    
    -- Balanço (VOLUME_FINAL - VOLUME_INICIAL)
    balanco_litros INT COMMENT 'volume_final - volume_inicial (+ entrada, - consumo)',
    
    -- Análise de vazamento
    consumo_esperado_litros INT COMMENT 'Consumo esperado para o período',
    consumo_anormal_litros INT COMMENT 'balanco negativo - consumo_esperado (vazamento se > 0)',
    percentual_anormal DECIMAL(5,2) COMMENT '(consumo_anormal / consumo_esperado) * 100',
    
    -- Vazões médias
    vazao_media_entrada_lpm DECIMAL(10,2) COMMENT 'Vazão média de entrada em L/min',
    vazao_media_saida_lpm DECIMAL(10,2) COMMENT 'Vazão média de saída em L/min',
    
    -- Metadata
    calculado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    recalculado_em TIMESTAMP NULL ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_reservatorio (reservatorio_id),
    INDEX idx_periodo (periodo_inicio, periodo_fim),
    UNIQUE KEY uk_reservatorio_periodo (reservatorio_id, periodo_inicio, periodo_fim)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Balanço hídrico calculado por período';

-- ============================================================================
-- 3. TABELA DE RELATÓRIOS DE SERVIÇO
-- ============================================================================
CREATE TABLE IF NOT EXISTS relatorios_servico (
    id INT AUTO_INCREMENT PRIMARY KEY,
    data_relatorio DATE NOT NULL,
    turno VARCHAR(20) COMMENT 'MANHA, TARDE, NOITE, 24H',
    operador VARCHAR(100) NOT NULL,
    supervisor VARCHAR(100),
    
    -- Status do sistema
    status_geral VARCHAR(50) DEFAULT 'NORMAL' COMMENT 'NORMAL, ALERTA, EMERGENCIA',
    
    -- Observações gerais
    condicoes_climaticas TEXT COMMENT 'Sol, chuva, temperatura, etc',
    ocorrencias TEXT COMMENT 'Eventos importantes do turno',
    manutencoes_realizadas TEXT COMMENT 'Serviços executados',
    pendencias TEXT COMMENT 'Tarefas pendentes',
    
    -- Dados automáticos (preenchidos por sensores)
    dados_sensores_json JSON COMMENT 'Snapshot dos níveis no momento do relatório',
    
    -- Validação
    validado BOOLEAN DEFAULT FALSE,
    validado_por VARCHAR(100),
    validado_em TIMESTAMP NULL,
    
    -- Metadata
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP NULL ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_data (data_relatorio),
    INDEX idx_operador (operador),
    INDEX idx_validado (validado)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Relatórios de serviço dos operadores';

-- ============================================================================
-- 4. TABELA DE ITENS DO RELATÓRIO (DETALHAMENTO POR RESERVATÓRIO)
-- ============================================================================
CREATE TABLE IF NOT EXISTS relatorio_reservatorios (
    id INT AUTO_INCREMENT PRIMARY KEY,
    relatorio_id INT NOT NULL,
    reservatorio_id VARCHAR(20) NOT NULL,
    
    -- Leituras (automáticas ou manuais)
    nivel_inicial_cm INT COMMENT 'Nível no início do turno',
    nivel_final_cm INT COMMENT 'Nível no fim do turno',
    percentual_inicial INT COMMENT '0-100%',
    percentual_final INT COMMENT '0-100%',
    volume_inicial_litros INT,
    volume_final_litros INT,
    
    -- Consumo calculado
    consumo_litros INT COMMENT 'volume_inicial - volume_final',
    consumo_m3 DECIMAL(10,2) COMMENT 'consumo_litros / 1000',
    
    -- Abastecimento recebido
    abastecimento_litros INT DEFAULT 0,
    horario_abastecimento VARCHAR(50) COMMENT 'Ex: 08:30-09:45',
    bomba_utilizada VARCHAR(50),
    
    -- Status
    estado_operacional VARCHAR(50) DEFAULT 'NORMAL' COMMENT 'NORMAL, ALERTA, CRITICO, MANUTENCAO',
    valvula_entrada VARCHAR(20) COMMENT 'ABERTA, FECHADA, PARCIAL',
    valvula_saida VARCHAR(20),
    
    -- Observações específicas
    observacoes TEXT,
    
    -- Fonte dos dados
    dados_automaticos BOOLEAN DEFAULT FALSE COMMENT 'TRUE se preenchido por sensor',
    
    FOREIGN KEY (relatorio_id) REFERENCES relatorios_servico(id) ON DELETE CASCADE,
    INDEX idx_relatorio (relatorio_id),
    INDEX idx_reservatorio (reservatorio_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Detalhamento por reservatório em cada relatório';

-- ============================================================================
-- 5. VIEW: BALANÇO DIÁRIO CONSOLIDADO
-- ============================================================================
CREATE OR REPLACE VIEW vw_balanco_diario AS
SELECT 
    DATE(periodo_inicio) as data,
    reservatorio_id,
    SUM(entrada_total_litros) as entrada_dia_litros,
    SUM(consumo_litros) as consumo_dia_litros,
    SUM(balanco_litros) as balanco_dia_litros,
    SUM(consumo_anormal_litros) as vazamento_dia_litros,
    AVG(percentual_anormal) as percentual_anormal_medio,
    COUNT(*) as num_periodos
FROM balanco_hidrico
GROUP BY DATE(periodo_inicio), reservatorio_id
ORDER BY data DESC, reservatorio_id;

-- ============================================================================
-- 6. VIEW: RELATÓRIOS PENDENTES DE VALIDAÇÃO
-- ============================================================================
CREATE OR REPLACE VIEW vw_relatorios_pendentes AS
SELECT 
    r.id,
    r.data_relatorio,
    r.turno,
    r.operador,
    r.status_geral,
    COUNT(rr.id) as num_reservatorios,
    DATEDIFF(CURDATE(), r.data_relatorio) as dias_atraso
FROM relatorios_servico r
LEFT JOIN relatorio_reservatorios rr ON r.id = rr.relatorio_id
WHERE r.validado = FALSE
GROUP BY r.id
HAVING dias_atraso <= 7
ORDER BY r.data_relatorio DESC;

-- ============================================================================
-- 7. STORED PROCEDURE: CALCULAR BALANÇO HÍDRICO
-- ============================================================================
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS calcular_balanco_hidrico(
    IN p_reservatorio VARCHAR(20),
    IN p_inicio TIMESTAMP,
    IN p_fim TIMESTAMP
)
BEGIN
    DECLARE v_volume_inicial INT;
    DECLARE v_volume_final INT;
    DECLARE v_entrada_total INT;
    DECLARE v_num_entradas INT;
    DECLARE v_balanco INT;
    DECLARE v_consumo INT;
    DECLARE v_consumo_esperado INT;
    DECLARE v_consumo_anormal INT;
    DECLARE v_pct_anormal DECIMAL(5,2);
    
    -- 1. Buscar volume inicial (primeira leitura do período)
    SELECT volume_l INTO v_volume_inicial
    FROM leituras_v2 l
    INNER JOIN sensores s ON l.node_id = s.node_id
    WHERE s.alias = p_reservatorio
    AND l.created_at >= p_inicio
    ORDER BY l.created_at ASC
    LIMIT 1;
    
    -- 2. Buscar volume final (última leitura do período)
    SELECT volume_l INTO v_volume_final
    FROM leituras_v2 l
    INNER JOIN sensores s ON l.node_id = s.node_id
    WHERE s.alias = p_reservatorio
    AND l.created_at <= p_fim
    ORDER BY l.created_at DESC
    LIMIT 1;
    
    -- 3. Calcular entradas (abastecimentos recebidos)
    SELECT 
        COALESCE(SUM(volume_litros), 0),
        COUNT(*)
    INTO v_entrada_total, v_num_entradas
    FROM eventos_abastecimento
    WHERE reservatorio_destino = p_reservatorio
    AND datetime BETWEEN p_inicio AND p_fim;
    
    -- 4. Calcular BALANÇO (variação real)
    -- BALANÇO = VOLUME_FINAL - VOLUME_INICIAL
    SET v_balanco = v_volume_final - v_volume_inicial;
    
    -- 5. Interpretar balanço:
    -- Se balanço > 0: ENTRADA (abastecimento)
    -- Se balanço < 0: SAÍDA (consumo)
    SET v_consumo = IF(v_balanco < 0, ABS(v_balanco), 0);
    
    -- 6. Buscar consumo esperado (média dos últimos 7 dias no mesmo período)
    -- TODO: Implementar cálculo de média histórica
    -- Por enquanto, assumir 10000 L/turno como padrão
    SET v_consumo_esperado = 10000;
    
    -- 7. Detectar consumo anormal (possível vazamento)
    -- Se consumo real > consumo esperado + tolerância (20%)
    SET v_consumo_anormal = 0;
    IF v_consumo > (v_consumo_esperado * 1.2) THEN
        SET v_consumo_anormal = v_consumo - v_consumo_esperado;
    END IF;
    
    -- 8. Percentual de consumo anormal
    IF v_consumo_esperado > 0 AND v_consumo_anormal > 0 THEN
        SET v_pct_anormal = (v_consumo_anormal / v_consumo_esperado) * 100;
    ELSE
        SET v_pct_anormal = 0;
    END IF;
    
    -- 9. Inserir ou atualizar balanço
    INSERT INTO balanco_hidrico (
        reservatorio_id, periodo_inicio, periodo_fim,
        volume_inicial_litros, volume_final_litros,
        entrada_total_litros, entrada_eventos,
        consumo_litros, balanco_litros,
        consumo_esperado_litros, consumo_anormal_litros, percentual_anormal
    ) VALUES (
        p_reservatorio, p_inicio, p_fim,
        v_volume_inicial, v_volume_final,
        v_entrada_total, v_num_entradas,
        v_consumo, v_balanco,
        v_consumo_esperado, v_consumo_anormal, v_pct_anormal
    )
    ON DUPLICATE KEY UPDATE
        volume_inicial_litros = v_volume_inicial,
        volume_final_litros = v_volume_final,
        entrada_total_litros = v_entrada_total,
        entrada_eventos = v_num_entradas,
        consumo_litros = v_consumo,
        balanco_litros = v_balanco,
        consumo_esperado_litros = v_consumo_esperado,
        consumo_anormal_litros = v_consumo_anormal,
        percentual_anormal = v_pct_anormal,
        recalculado_em = CURRENT_TIMESTAMP;
END //
DELIMITER ;

-- ============================================================================
-- 8. DADOS DE EXEMPLO
-- ============================================================================

-- Exemplo de evento de abastecimento
INSERT IGNORE INTO eventos_abastecimento (
    datetime, reservatorio_origem, reservatorio_destino,
    volume_litros, duracao_minutos, bomba_utilizada, vazao_lpm, operador
) VALUES
    ('2025-12-14 08:30:00', 'RCB3', 'RCON', 15000, 25, 'BOR_CB3_ME1', 600, 'João Silva'),
    ('2025-12-14 14:45:00', 'RCB3', 'RCON', 12000, 20, 'BOR_CB3_ME1', 600, 'Maria Santos');

-- Exemplo de relatório de serviço
INSERT IGNORE INTO relatorios_servico (
    data_relatorio, turno, operador, supervisor, status_geral,
    condicoes_climaticas, ocorrencias
) VALUES
    ('2025-12-14', 'MANHA', 'João Silva', 'Carlos Oliveira', 'NORMAL',
     'Dia ensolarado, temperatura amena 22°C',
     'Operação normal. Abastecimento RCON realizado às 08:30h com bomba elétrica.');

-- ============================================================================
-- FIM DA MIGRATION 004
-- ============================================================================
