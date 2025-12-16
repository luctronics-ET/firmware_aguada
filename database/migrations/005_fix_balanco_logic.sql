-- Migração 005: Correção da Lógica de Balanço Hídrico
-- Data: 2025-12-15
-- Descrição: Corrige a fórmula BALANÇO = VOLUME_FINAL - VOLUME_INICIAL
--            e adiciona detecção de vazamentos por consumo anormal

USE sensores_db;

-- 1. Recriar tabela balanco_hidrico com estrutura correta
DROP TABLE IF EXISTS balanco_hidrico;

CREATE TABLE balanco_hidrico (
    id INT AUTO_INCREMENT PRIMARY KEY,
    reservatorio_id VARCHAR(20) NOT NULL,
    periodo_inicio DATETIME NOT NULL,
    periodo_fim DATETIME NOT NULL,
    
    -- Volumes medidos pelos sensores
    volume_inicial_litros INT COMMENT 'Volume no início do período',
    volume_final_litros INT COMMENT 'Volume no fim do período',
    
    -- Entradas (abastecimento registrado)
    entrada_total_litros INT DEFAULT 0 COMMENT 'Total de água abastecida (eventos registrados)',
    entrada_eventos INT DEFAULT 0 COMMENT 'Número de eventos de abastecimento',
    
    -- Consumo calculado
    consumo_litros INT COMMENT 'Consumo calculado (se balanço negativo)',
    
    -- Balanço (VOLUME_FINAL - VOLUME_INICIAL)
    balanco_litros INT COMMENT 'volume_final - volume_inicial (+ entrada, - consumo)',
    
    -- Análise de vazamento
    consumo_esperado_litros INT COMMENT 'Consumo esperado para o período',
    consumo_anormal_litros INT COMMENT 'balanco negativo - consumo_esperado (vazamento se > 0)',
    percentual_anormal DECIMAL(5,2) COMMENT '(consumo_anormal / consumo_esperado) * 100',
    
    -- Timestamps
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    recalculado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    -- Constraints
    UNIQUE KEY idx_periodo (reservatorio_id, periodo_inicio, periodo_fim),
    INDEX idx_data (periodo_inicio, periodo_fim),
    INDEX idx_reservatorio (reservatorio_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Balanço hídrico calculado por período';

-- 2. Atualizar comentário da coluna balanco_litros
ALTER TABLE balanco_hidrico 
    MODIFY COLUMN balanco_litros INT COMMENT 'volume_final - volume_inicial (+ entrada, - consumo)';

-- 3. Recriar a stored procedure com lógica correta
DROP PROCEDURE IF EXISTS calcular_balanco_hidrico;

DELIMITER //

CREATE PROCEDURE calcular_balanco_hidrico(
    IN p_reservatorio VARCHAR(20),
    IN p_inicio DATETIME,
    IN p_fim DATETIME
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
    SELECT CAST(value_int / 100 AS SIGNED) INTO v_volume_inicial
    FROM leituras_v2
    WHERE sensor_id = p_reservatorio 
      AND variable_type = 'volume_litros'
      AND datetime >= p_inicio
    ORDER BY datetime ASC
    LIMIT 1;
    
    -- 2. Buscar volume final (última leitura do período)
    SELECT CAST(value_int / 100 AS SIGNED) INTO v_volume_final
    FROM leituras_v2
    WHERE sensor_id = p_reservatorio 
      AND variable_type = 'volume_litros'
      AND datetime <= p_fim
    ORDER BY datetime DESC
    LIMIT 1;
    
    -- 3. Somar eventos de abastecimento no período
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
    
    -- 10. Retornar resultado
    SELECT 
        v_volume_inicial as volume_inicial,
        v_volume_final as volume_final,
        v_entrada_total as entrada_total,
        v_consumo as consumo,
        v_balanco as balanco,
        v_consumo_esperado as consumo_esperado,
        v_consumo_anormal as consumo_anormal,
        v_pct_anormal as percentual_anormal,
        CASE 
            WHEN v_balanco > 0 THEN 'ENTRADA (Abastecimento)'
            WHEN v_balanco < 0 THEN 'SAÍDA (Consumo)'
            ELSE 'ESTÁVEL'
        END as interpretacao,
        CASE 
            WHEN v_consumo_anormal > 0 THEN 'ALERTA: Possível vazamento detectado!'
            ELSE 'Normal'
        END as status_vazamento;
END //

DELIMITER ;

-- 4. Recriar view de balanço diário
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

-- 5. Criar view para detecção de vazamentos
CREATE OR REPLACE VIEW vw_alertas_vazamento AS
SELECT 
    b.reservatorio_id,
    b.periodo_inicio,
    b.periodo_fim,
    b.consumo_litros,
    b.consumo_esperado_litros,
    b.consumo_anormal_litros,
    b.percentual_anormal,
    s.alias as nome_reservatorio,
    s.elemento_id,
    CASE 
        WHEN b.percentual_anormal >= 50 THEN 'CRÍTICO'
        WHEN b.percentual_anormal >= 20 THEN 'ALERTA'
        ELSE 'NORMAL'
    END as nivel_alerta
FROM balanco_hidrico b
LEFT JOIN sensores s ON b.reservatorio_id = s.alias COLLATE utf8mb4_unicode_ci
WHERE b.consumo_anormal_litros > 0
ORDER BY b.percentual_anormal DESC, b.periodo_inicio DESC;

-- 6. Inserir exemplo de teste
-- Simular um período com vazamento
INSERT INTO eventos_abastecimento (
    datetime, reservatorio_origem, reservatorio_destino,
    volume_litros, duracao_minutos, bomba_utilizada, vazao_lpm, operador
) VALUES (
    '2025-12-15 08:00:00', 'RCAV', 'RCON',
    5000, 30, 'ME1', 167, 'João Silva'
);

-- Comentários finais
SELECT 'Migração 005 aplicada com sucesso!' as status;
SELECT 'Nova lógica: BALANÇO = VOLUME_FINAL - VOLUME_INICIAL' as formula;
SELECT 'Balanço > 0 = ENTRADA (abastecimento)' as interpretacao_positiva;
SELECT 'Balanço < 0 = SAÍDA (consumo)' as interpretacao_negativa;
SELECT 'Vazamento detectado quando consumo > consumo_esperado * 1.2' as deteccao_vazamento;
