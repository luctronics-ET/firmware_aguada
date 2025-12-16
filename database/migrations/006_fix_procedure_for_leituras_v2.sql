-- Migração 006: Corrigir stored procedure para usar leituras_v2
-- Data: 2025-12-15

USE sensores_db;

DROP PROCEDURE IF EXISTS calcular_balanco_hidrico;

DELIMITER //

CREATE PROCEDURE calcular_balanco_hidrico(
    IN p_reservatorio VARCHAR(20),  -- Pode ser node_id ou sensor alias
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
    DECLARE v_node_id SMALLINT;
    
    -- Converter alias para node_id se necessário
    IF p_reservatorio REGEXP '^[0-9]+$' THEN
        -- É um node_id numérico
        SET v_node_id = CAST(p_reservatorio AS UNSIGNED);
    ELSE
        -- É um alias, buscar node_id
        SELECT node_id INTO v_node_id
        FROM sensores
        WHERE alias = p_reservatorio
        LIMIT 1;
    END IF;
    
    -- 1. Buscar volume inicial (primeira leitura do período)
    -- volume_l já está em litros na leituras_v2
    SELECT COALESCE(volume_l, 0) INTO v_volume_inicial
    FROM leituras_v2
    WHERE node_id = v_node_id 
      AND volume_l IS NOT NULL
      AND created_at >= p_inicio
    ORDER BY created_at ASC
    LIMIT 1;
    
    -- 2. Buscar volume final (última leitura do período)
    SELECT COALESCE(volume_l, 0) INTO v_volume_final
    FROM leituras_v2
    WHERE node_id = v_node_id 
      AND volume_l IS NOT NULL
      AND created_at <= p_fim
    ORDER BY created_at DESC
    LIMIT 1;
    
    -- 3. Somar eventos de abastecimento no período
    -- Tentar match por node_id ou por alias
    SELECT 
        COALESCE(SUM(volume_litros), 0),
        COUNT(*)
    INTO v_entrada_total, v_num_entradas
    FROM eventos_abastecimento
    WHERE (
        reservatorio_destino COLLATE utf8mb4_unicode_ci = p_reservatorio OR
        reservatorio_destino = CAST(v_node_id AS CHAR)
    )
    AND datetime BETWEEN p_inicio AND p_fim;
    
    -- 4. Calcular BALANÇO (variação real)
    -- BALANÇO = VOLUME_FINAL - VOLUME_INICIAL
    SET v_balanco = v_volume_final - v_volume_inicial;
    
    -- 5. Interpretar balanço:
    -- Se balanço > 0: ENTRADA (abastecimento)
    -- Se balanço < 0: SAÍDA (consumo)
    SET v_consumo = IF(v_balanco < 0, ABS(v_balanco), 0);
    
    -- 6. Buscar consumo esperado (média dos últimos 7 dias no mesmo período)
    -- TODO: Implementar cálculo de média histórica baseado em horário
    -- Por enquanto, calcular média simples dos últimos 7 dias
    SELECT AVG(consumo_medio) INTO v_consumo_esperado
    FROM (
        SELECT DATE(created_at) as dia, 
               ABS(MIN(volume_l) - MAX(volume_l)) as consumo_medio
        FROM leituras_v2
        WHERE node_id = v_node_id
          AND created_at >= DATE_SUB(p_inicio, INTERVAL 7 DAY)
          AND created_at < p_inicio
          AND volume_l IS NOT NULL
        GROUP BY DATE(created_at)
        HAVING consumo_medio > 0
    ) historico;
    
    -- Se não há histórico, assumir padrão de 10000 L/dia
    IF v_consumo_esperado IS NULL OR v_consumo_esperado = 0 THEN
        SET v_consumo_esperado = 10000;
    END IF;
    
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
    
    -- 10. Retornar resultado detalhado
    SELECT 
        p_reservatorio as reservatorio,
        v_node_id as node_id,
        v_volume_inicial as volume_inicial_litros,
        v_volume_final as volume_final_litros,
        v_entrada_total as entrada_litros,
        v_num_entradas as num_abastecimentos,
        v_consumo as consumo_litros,
        v_balanco as balanco_litros,
        v_consumo_esperado as consumo_esperado_litros,
        v_consumo_anormal as consumo_anormal_litros,
        v_pct_anormal as percentual_anormal,
        CASE 
            WHEN v_balanco > 0 THEN 'ENTRADA (Abastecimento)'
            WHEN v_balanco < 0 THEN 'SAÍDA (Consumo)'
            ELSE 'ESTÁVEL'
        END as interpretacao,
        CASE 
            WHEN v_consumo_anormal > 0 AND v_pct_anormal >= 50 THEN 'CRÍTICO: Vazamento severo!'
            WHEN v_consumo_anormal > 0 AND v_pct_anormal >= 20 THEN 'ALERTA: Possível vazamento'
            WHEN v_consumo_anormal > 0 THEN 'ATENÇÃO: Consumo acima do esperado'
            ELSE 'NORMAL'
        END as status_vazamento;
END //

DELIMITER ;

-- Testar com dados reais
SELECT 'Procedure recriada com sucesso!' as status;
SELECT 'Testando com node_id = 1 (RCON)...' as teste;

CALL calcular_balanco_hidrico('1', '2025-12-14 00:00:00', '2025-12-14 23:59:59');
