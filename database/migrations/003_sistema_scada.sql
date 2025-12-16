-- Schema v2 - Sistema SCADA Hidráulico Completo
-- Migração: Adiciona modelagem de instalações e elementos hidráulicos

-- ====================
-- LOCALIZAÇÕES E INSTALAÇÕES
-- ====================

CREATE TABLE locais (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nome VARCHAR(100) NOT NULL UNIQUE COMMENT 'Ex: Castelo de Consumo, Castelo de Incêndio',
    descricao TEXT,
    latitude DECIMAL(10, 8),
    longitude DECIMAL(11, 8),
    altitude_m DECIMAL(6, 2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_nome (nome)
) COMMENT 'Locais físicos onde sensores/elementos estão instalados';

-- ====================
-- ELEMENTOS HIDRÁULICOS
-- ====================

CREATE TABLE tipos_elemento (
    id INT AUTO_INCREMENT PRIMARY KEY,
    codigo VARCHAR(20) NOT NULL UNIQUE COMMENT 'reservatorio_elevado, cisterna, bomba_recalque, etc',
    nome VARCHAR(50) NOT NULL COMMENT 'Nome legível do tipo',
    categoria ENUM('armazenamento', 'transporte', 'controle', 'medicao', 'pressao') NOT NULL,
    num_entradas_padrao TINYINT DEFAULT 0 COMMENT 'Número típico de entradas',
    num_saidas_padrao TINYINT DEFAULT 0 COMMENT 'Número típico de saídas',
    icone VARCHAR(50) COMMENT 'Nome do ícone para UI',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) COMMENT 'Tipos de elementos do sistema hidráulico';

-- Inserir tipos padrão
INSERT INTO tipos_elemento (codigo, nome, categoria, num_entradas_padrao, num_saidas_padrao, icone) VALUES
('reservatorio_elevado', 'Reservatório Elevado', 'armazenamento', 1, 1, 'water-tower'),
('reservatorio', 'Reservatório', 'armazenamento', 1, 1, 'water-tank'),
('cisterna', 'Cisterna (Subterrânea)', 'armazenamento', 1, 1, 'water-underground'),
('encanamento', 'Encanamento', 'transporte', 1, 1, 'pipe'),
('valvula', 'Válvula', 'controle', 1, 1, 'valve'),
('valvula_y', 'Válvula Y (1→2)', 'controle', 1, 2, 'valve-y'),
('valvula_3vias', 'Válvula 3 Vias', 'controle', 1, 2, 'valve-3way'),
('bomba_pressao', 'Bomba de Pressão', 'pressao', 1, 1, 'pump-pressure'),
('bomba_recalque', 'Bomba de Recalque', 'pressao', 1, 1, 'pump-recalque'),
('hidrometro', 'Hidrômetro', 'medicao', 1, 1, 'water-meter'),
('registro', 'Registro', 'controle', 1, 1, 'register'),
('boia', 'Boia de Nível', 'controle', 0, 0, 'float-switch');

-- ====================
-- REDES DE ÁGUA (por qualidade/finalidade)
-- ====================

CREATE TABLE redes_agua (
    id INT AUTO_INCREMENT PRIMARY KEY,
    codigo VARCHAR(50) NOT NULL UNIQUE COMMENT 'REDE_CONSUMO, REDE_INCENDIO, etc',
    nome VARCHAR(100) NOT NULL,
    finalidade TEXT COMMENT 'Descrição da finalidade da rede',
    qualidade_agua ENUM('potavel', 'nao_potavel', 'industrial', 'reuso') DEFAULT 'nao_potavel',
    pressao_minima_bar DECIMAL(5, 2) COMMENT 'Pressão mínima operacional (bar)',
    pressao_maxima_bar DECIMAL(5, 2) COMMENT 'Pressão máxima segura (bar)',
    cor_diagrama VARCHAR(7) COMMENT 'Cor hexadecimal para visualização (#0066CC)',
    norma_aplicavel VARCHAR(100) COMMENT 'Norma técnica aplicável (NBR 13714, etc)',
    ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_codigo (codigo)
) COMMENT 'Redes de água separadas por qualidade/finalidade';

-- Inserir redes padrão
INSERT INTO redes_agua (codigo, nome, finalidade, qualidade_agua, pressao_minima_bar, pressao_maxima_bar, cor_diagrama, norma_aplicavel) VALUES
('REDE_CONSUMO', 'Rede de Consumo', 'Abastecimento residencial (torneiras, chuveiros)', 'potavel', 1.0, 4.0, '#0066CC', 'NBR 5626'),
('REDE_INCENDIO', 'Rede de Incêndio', 'Combate a incêndio (hidrantes Y, mangueiras)', 'nao_potavel', 4.0, 8.0, '#FF0000', 'NBR 13714'),
('REDE_PISCINA', 'Rede de Piscina', 'Recirculação e tratamento de piscina', 'nao_potavel', 0.5, 2.0, '#00CCCC', 'NBR 10818'),
('REDE_REUSO', 'Rede de Reuso', 'Água de chuva/cinza para descarga e irrigação', 'reuso', 0.5, 2.0, '#996600', 'NBR 15527');

CREATE TABLE elementos (
    id INT AUTO_INCREMENT PRIMARY KEY,
    alias VARCHAR(100) NOT NULL UNIQUE COMMENT 'Código único: ULTRA_CASTELO_CONSUMO, BOMBA_RECALQUE_01',
    nome VARCHAR(100) NOT NULL COMMENT 'Ex: Reservatório Consumo, Bomba Principal',
    tipo_id INT NOT NULL,
    local_id INT NOT NULL,
    rede_agua_id INT COMMENT 'Rede à qual este elemento pertence',
    
    -- Características físicas
    capacidade_l INT COMMENT 'Capacidade em litros (para reservatórios)',
    altura_m DECIMAL(6, 2) COMMENT 'Altura física do elemento (metros)',
    diametro_cm DECIMAL(6, 2) COMMENT 'Diâmetro (cm) - para reservatórios cilíndricos',
    comprimento_m DECIMAL(8, 2) COMMENT 'Comprimento (m) - para encanamentos',
    diametro_polegadas DECIMAL(4, 2) COMMENT 'Diâmetro nominal (polegadas) - para tubos/válvulas',
    
    -- Estado operacional
    estado_operacional ENUM('ativo', 'inativo', 'manutencao', 'falha') DEFAULT 'ativo',
    
    -- Metadata
    fabricante VARCHAR(100),
    modelo VARCHAR(100),
    ano_instalacao YEAR,
    observacoes TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (tipo_id) REFERENCES tipos_elemento(id),
    FOREIGN KEY (local_id) REFERENCES locais(id),
    FOREIGN KEY (rede_agua_id) REFERENCES redes_agua(id),
    INDEX idx_alias (alias),
    INDEX idx_tipo (tipo_id),
    INDEX idx_local (local_id),
    INDEX idx_rede (rede_agua_id),
    INDEX idx_estado (estado_operacional)
) COMMENT 'Elementos individuais do sistema hidráulico';

-- ====================
-- CONEXÕES ENTRE ELEMENTOS
-- ====================

CREATE TABLE conexoes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    elemento_origem_id INT NOT NULL,
    elemento_destino_id INT NOT NULL,
    porta_origem TINYINT DEFAULT 1 COMMENT 'Número da saída no elemento origem (1-N)',
    porta_destino TINYINT DEFAULT 1 COMMENT 'Número da entrada no elemento destino (1-N)',
    
    -- Características da conexão
    tipo_conexao ENUM('rosca', 'flange', 'soldada', 'ppr', 'pex') COMMENT 'Tipo de conexão física',
    diametro_polegadas DECIMAL(4, 2),
    comprimento_m DECIMAL(8, 2),
    material VARCHAR(50) COMMENT 'PVC, ferro galvanizado, cobre, etc',
    
    ativo BOOLEAN DEFAULT TRUE,
    observacoes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (elemento_origem_id) REFERENCES elementos(id) ON DELETE CASCADE,
    FOREIGN KEY (elemento_destino_id) REFERENCES elementos(id) ON DELETE CASCADE,
    INDEX idx_origem (elemento_origem_id),
    INDEX idx_destino (elemento_destino_id),
    
    -- Evitar duplicatas
    UNIQUE KEY unique_conexao (elemento_origem_id, elemento_destino_id, porta_origem, porta_destino)
) COMMENT 'Conexões físicas entre elementos do sistema';

-- ====================
-- ESTADOS DOS ELEMENTOS
-- ====================

CREATE TABLE estados_valvula (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    elemento_id INT NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    estado ENUM('aberta', 'fechada', 'parcial') NOT NULL,
    percentual_abertura TINYINT COMMENT '0-100% de abertura',
    modo ENUM('manual', 'automatico') DEFAULT 'manual',
    
    -- Quem comandou
    usuario_id INT COMMENT 'ID do usuário que alterou (se manual)',
    motivo VARCHAR(255) COMMENT 'Motivo da alteração',
    
    FOREIGN KEY (elemento_id) REFERENCES elementos(id) ON DELETE CASCADE,
    INDEX idx_elemento_timestamp (elemento_id, timestamp)
) COMMENT 'Histórico de estados de válvulas';

CREATE TABLE estados_bomba (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    elemento_id INT NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    estado ENUM('ligada', 'desligada', 'falha') NOT NULL,
    frequencia_hz DECIMAL(5, 2) COMMENT 'Frequência do inversor (Hz)',
    corrente_a DECIMAL(6, 2) COMMENT 'Corrente elétrica (A)',
    pressao_bar DECIMAL(6, 2) COMMENT 'Pressão de saída (bar)',
    vazao_lh DECIMAL(8, 2) COMMENT 'Vazão (L/h)',
    temperatura_c DECIMAL(5, 2) COMMENT 'Temperatura do motor (°C)',
    
    modo ENUM('manual', 'automatico') DEFAULT 'manual',
    usuario_id INT,
    motivo VARCHAR(255),
    
    FOREIGN KEY (elemento_id) REFERENCES elementos(id) ON DELETE CASCADE,
    INDEX idx_elemento_timestamp (elemento_id, timestamp)
) COMMENT 'Histórico de estados de bombas';

-- ====================
-- ASSOCIAÇÃO SENSORES → ELEMENTOS
-- ====================

CREATE TABLE sensores (
    id INT AUTO_INCREMENT PRIMARY KEY,
    alias VARCHAR(100) NOT NULL UNIQUE COMMENT 'Código único: ULTRA_CASTELO_CONSUMO, NANO_CISTERNA_SEC',
    node_id SMALLINT NOT NULL UNIQUE COMMENT 'Node ID do firmware (1-255)',
    mac VARCHAR(17) NOT NULL UNIQUE,
    tipo_sensor ENUM('ultrasonic', 'pressure', 'flow', 'temperature') NOT NULL,
    
    -- Associação ao elemento (NULL = sem elemento associado ainda)
    elemento_id INT COMMENT 'Elemento que este sensor monitora (pode ter múltiplos sensores)',
    posicao_sensor VARCHAR(50) COMMENT 'Posição física: topo, base, entrada, saída',
    
    -- Configuração
    offset_cm INT DEFAULT 0,
    intervalo_leitura_s INT DEFAULT 30,
    ativo BOOLEAN DEFAULT TRUE,
    
    -- Metadata
    fabricante VARCHAR(100),
    modelo VARCHAR(100),
    data_instalacao DATE,
    data_ultima_calibracao DATE,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (elemento_id) REFERENCES elementos(id),
    INDEX idx_alias (alias),
    INDEX idx_elemento (elemento_id),
    INDEX idx_node (node_id),
    INDEX idx_mac (mac)
) COMMENT 'Registro de sensores - podem ter múltiplos sensores por elemento';

-- Migrar dados existentes de leituras_v2 para nova estrutura
-- (assumindo que node_id 1-5 são sensores ultrassônicos em reservatórios)

-- ====================
-- LEITURAS (mantém estrutura atual, adiciona FK para sensor)
-- ====================

-- ALTER TABLE leituras_v2 
--     ADD COLUMN sensor_id INT COMMENT 'FK para tabela sensores',
--     ADD FOREIGN KEY (sensor_id) REFERENCES sensores(id);
-- Nota: Campos sensor_id já existe, será vinculado manualmente depois

-- ====================
-- ANOMALIAS E ALERTAS
-- ====================

CREATE TABLE anomalias (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    elemento_id INT NOT NULL,
    tipo ENUM('vazamento', 'entupimento', 'queda_pressao', 'nivel_critico', 'bomba_falha', 'sensor_offline') NOT NULL,
    severidade ENUM('info', 'aviso', 'critico') NOT NULL,
    
    timestamp_inicio TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    timestamp_fim TIMESTAMP NULL,
    resolvido BOOLEAN DEFAULT FALSE,
    
    descricao TEXT,
    valor_detectado DECIMAL(10, 2) COMMENT 'Valor que gerou a anomalia',
    valor_esperado DECIMAL(10, 2) COMMENT 'Valor esperado/normal',
    
    acao_tomada TEXT COMMENT 'Ação corretiva aplicada',
    usuario_responsavel_id INT,
    
    FOREIGN KEY (elemento_id) REFERENCES elementos(id),
    INDEX idx_elemento (elemento_id),
    INDEX idx_resolvido (resolvido),
    INDEX idx_timestamp (timestamp_inicio)
) COMMENT 'Registro de anomalias detectadas no sistema';

-- ====================
-- VIEWS ÚTEIS
-- ====================

-- View: Status atual de todos os elementos
CREATE VIEW vw_status_elementos AS
SELECT 
    e.id,
    e.alias AS elemento_alias,
    e.nome AS elemento_nome,
    te.nome AS tipo_elemento,
    te.categoria,
    l.nome AS local_nome,
    e.estado_operacional,
    e.capacidade_l,
    
    -- Informações da rede
    r.codigo AS rede_codigo,
    r.nome AS rede_nome,
    r.cor_diagrama AS rede_cor,
    
    -- Última leitura de sensor (se houver) - pode ter múltiplos sensores
    s.alias AS sensor_alias,
    s.node_id AS sensor_node_id,
    s.tipo_sensor,
    (SELECT level_cm FROM leituras_v2 WHERE node_id = s.node_id ORDER BY created_at DESC LIMIT 1) AS nivel_atual_cm,
    (SELECT percentual FROM leituras_v2 WHERE node_id = s.node_id ORDER BY created_at DESC LIMIT 1) AS percentual_nivel,
    (SELECT volume_l FROM leituras_v2 WHERE node_id = s.node_id ORDER BY created_at DESC LIMIT 1) AS volume_atual_l,
    (SELECT created_at FROM leituras_v2 WHERE node_id = s.node_id ORDER BY created_at DESC LIMIT 1) AS ultima_leitura,
    
    -- Estado de válvula (se aplicável)
    (SELECT estado FROM estados_valvula WHERE elemento_id = e.id ORDER BY timestamp DESC LIMIT 1) AS valvula_estado,
    (SELECT percentual_abertura FROM estados_valvula WHERE elemento_id = e.id ORDER BY timestamp DESC LIMIT 1) AS valvula_percentual,
    
    -- Estado de bomba (se aplicável)
    (SELECT estado FROM estados_bomba WHERE elemento_id = e.id ORDER BY timestamp DESC LIMIT 1) AS bomba_estado,
    (SELECT frequencia_hz FROM estados_bomba WHERE elemento_id = e.id ORDER BY timestamp DESC LIMIT 1) AS bomba_frequencia,
    (SELECT corrente_a FROM estados_bomba WHERE elemento_id = e.id ORDER BY timestamp DESC LIMIT 1) AS bomba_corrente,
    (SELECT pressao_bar FROM estados_bomba WHERE elemento_id = e.id ORDER BY timestamp DESC LIMIT 1) AS bomba_pressao,
    
    -- Anomalias ativas
    (SELECT COUNT(*) FROM anomalias WHERE elemento_id = e.id AND resolvido = FALSE) AS anomalias_ativas
    
FROM elementos e
INNER JOIN tipos_elemento te ON e.tipo_id = te.id
INNER JOIN locais l ON e.local_id = l.id
LEFT JOIN redes_agua r ON e.rede_agua_id = r.id
LEFT JOIN sensores s ON s.elemento_id = e.id;

-- View: Mapa de conexões (grafo do sistema)
CREATE VIEW vw_mapa_sistema AS
SELECT 
    c.id AS conexao_id,
    eo.alias AS origem_alias,
    eo.nome AS origem_nome,
    teo.nome AS origem_tipo,
    ed.alias AS destino_alias,
    ed.nome AS destino_nome,
    ted.nome AS destino_tipo,
    c.porta_origem,
    c.porta_destino,
    c.tipo_conexao,
    c.diametro_polegadas,
    c.comprimento_m,
    c.material,
    c.ativo,
    r.codigo AS rede_codigo,
    r.cor_diagrama AS rede_cor
FROM conexoes c
INNER JOIN elementos eo ON c.elemento_origem_id = eo.id
INNER JOIN elementos ed ON c.elemento_destino_id = ed.id
INNER JOIN tipos_elemento teo ON eo.tipo_id = teo.id
INNER JOIN tipos_elemento ted ON ed.tipo_id = ted.id
LEFT JOIN redes_agua r ON eo.rede_agua_id = r.id;

-- View: Anomalias ativas por local
CREATE VIEW vw_anomalias_ativas AS
SELECT 
    l.nome AS local_nome,
    e.alias AS elemento_alias,
    e.nome AS elemento_nome,
    a.tipo AS anomalia_tipo,
    a.severidade,
    a.timestamp_inicio,
    a.descricao,
    a.valor_detectado,
    a.valor_esperado,
    TIMESTAMPDIFF(HOUR, a.timestamp_inicio, NOW()) AS horas_aberta
FROM anomalias a
INNER JOIN elementos e ON a.elemento_id = e.id
INNER JOIN locais l ON e.local_id = l.id
WHERE a.resolvido = FALSE
ORDER BY a.severidade DESC, a.timestamp_inicio DESC;

-- ====================
-- TRIGGERS
-- ====================

-- Trigger: Detectar anomalia de nível crítico
DELIMITER //
DROP TRIGGER IF EXISTS trg_anomalia_nivel_critico //
CREATE DEFINER=`aguada_user`@`localhost` TRIGGER trg_anomalia_nivel_critico
AFTER INSERT ON leituras_v2
FOR EACH ROW
BEGIN
    DECLARE elem_id INT;
    DECLARE capacidade INT;
    
    -- Obter elemento associado ao sensor
    SELECT s.elemento_id, e.capacidade_l 
    INTO elem_id, capacidade
    FROM sensores s
    INNER JOIN elementos e ON s.elemento_id = e.id
    WHERE s.node_id = NEW.node_id
    LIMIT 1;
    
    -- Inserir anomalia se nível < 10% ou > 95%
    IF elem_id IS NOT NULL THEN
        IF NEW.percentual < 10 THEN
            INSERT INTO anomalias (elemento_id, tipo, severidade, descricao, valor_detectado, valor_esperado)
            VALUES (elem_id, 'nivel_critico', 'critico', 'Nível abaixo de 10%', NEW.percentual, 50)
            ON DUPLICATE KEY UPDATE timestamp_inicio = NOW();
        ELSEIF NEW.percentual > 95 THEN
            INSERT INTO anomalias (elemento_id, tipo, severidade, descricao, valor_detectado, valor_esperado)
            VALUES (elem_id, 'nivel_critico', 'aviso', 'Nível acima de 95%', NEW.percentual, 80)
            ON DUPLICATE KEY UPDATE timestamp_inicio = NOW();
        END IF;
    END IF;
END//
DELIMITER ;

-- ====================
-- DADOS DE EXEMPLO
-- ====================

-- Locais (Sistema Real)
INSERT INTO locais (nome, descricao, latitude, longitude, altitude_m) VALUES
('Ilha das Flores', 'Cisternas IF1/IF2 + Casa Bombas IF + Reservatório RCIF', -23.550120, -46.632808, 5.0),
('Ilha do Engenho', 'Cisternas IE1/IE2 principais', -23.550420, -46.633108, 3.0),
('Casa de Bombas N03', 'Bombas diesel/elétrica + RCB3 + Válvula Y', -23.550320, -46.633208, 8.0),
('Castelo de Consumo', 'Reservatório RCON elevado', -23.550520, -46.633308, 120.0),
('Castelo de Incêndio', 'Reservatório RCAV elevado', -23.550620, -46.633408, 125.0);

-- Elementos do Sistema Real (alias conforme mapeamento)

-- Ilha das Flores - Entrada do Sistema
INSERT INTO elementos (alias, nome, tipo_id, local_id, rede_agua_id, capacidade_l, altura_m, diametro_cm, estado_operacional) VALUES
('CIF1', 'Cisterna Ilha Flores 01', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'cisterna'), 
    (SELECT id FROM locais WHERE nome = 'Ilha das Flores'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    80000, 3.0, 220, 'ativo'),

('CIF2', 'Cisterna Ilha Flores 02', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'cisterna'), 
    (SELECT id FROM locais WHERE nome = 'Ilha das Flores'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    80000, 3.0, 220, 'ativo'),

('RCIF', 'Reservatório Casa Bombas Ilha Flores (80m³)', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'reservatorio'), 
    (SELECT id FROM locais WHERE nome = 'Ilha das Flores'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    80000, 4.0, 200, 'ativo');

-- Ilha do Engenho - Cisternas Principais
INSERT INTO elementos (alias, nome, tipo_id, local_id, rede_agua_id, capacidade_l, altura_m, diametro_cm, estado_operacional) VALUES
('CIE1', 'Cisterna Ilha Engenho 01', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'cisterna'), 
    (SELECT id FROM locais WHERE nome = 'Ilha do Engenho'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    120000, 3.5, 250, 'ativo'),

('CIE2', 'Cisterna Ilha Engenho 02', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'cisterna'), 
    (SELECT id FROM locais WHERE nome = 'Ilha do Engenho'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    120000, 3.5, 250, 'ativo');

-- Casa de Bombas N03 - Centro de Distribuição
INSERT INTO elementos (alias, nome, tipo_id, local_id, rede_agua_id, capacidade_l, altura_m, diametro_cm, estado_operacional) VALUES
('RCB3', 'Reservatório Casa Bombas N03', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'reservatorio'), 
    (SELECT id FROM locais WHERE nome = 'Casa de Bombas N03'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    50000, 3.0, 180, 'ativo'),

('BOR_CB3_MD1', 'Bomba Recalque CB3 - Motor Diesel 01', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'bomba_recalque'), 
    (SELECT id FROM locais WHERE nome = 'Casa de Bombas N03'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    NULL, NULL, NULL, 'ativo'),

('BOR_CB3_ME1', 'Bomba Recalque CB3 - Motor Elétrico 01', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'bomba_recalque'), 
    (SELECT id FROM locais WHERE nome = 'Casa de Bombas N03'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    NULL, NULL, NULL, 'ativo'),

('VALVY_RCB3_OUT1', 'Válvula Y Distribuição RCB3', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'valvula_y'), 
    (SELECT id FROM locais WHERE nome = 'Casa de Bombas N03'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    NULL, NULL, NULL, 'ativo');

-- Castelos - Reservatórios Elevados Finais
INSERT INTO elementos (alias, nome, tipo_id, local_id, rede_agua_id, capacidade_l, altura_m, diametro_cm, estado_operacional) VALUES
('RCON', 'Reservatório Castelo Consumo', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'reservatorio_elevado'), 
    (SELECT id FROM locais WHERE nome = 'Castelo de Consumo'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_CONSUMO'),
    80000, 4.5, 200, 'ativo'),

('RCAV', 'Reservatório Castelo Incêndio', 
    (SELECT id FROM tipos_elemento WHERE codigo = 'reservatorio_elevado'), 
    (SELECT id FROM locais WHERE nome = 'Castelo de Incêndio'),
    (SELECT id FROM redes_agua WHERE codigo = 'REDE_INCENDIO'),
    80000, 3.8, 220, 'ativo');

-- Sensores Instalados (MACs reais conforme mapeamento)
INSERT INTO sensores (alias, node_id, mac, tipo_sensor, elemento_id, posicao_sensor, offset_cm, ativo) VALUES
-- ESP32 Sensors com alias conforme doc
('RCON', 1, '20:6E:F1:6B:77:58', 'ultrasonic', 
    (SELECT id FROM elementos WHERE alias = 'RCON'), 
    'topo', 20, TRUE),

('RCAV', 2, 'DC:06:75:67:6A:CC', 'ultrasonic', 
    (SELECT id FROM elementos WHERE alias = 'RCAV'), 
    'topo', 20, TRUE),

('RCB3', 3, '80:F1:B2:50:31:34', 'ultrasonic', 
    (SELECT id FROM elementos WHERE alias = 'RCB3'), 
    'topo', 15, TRUE),

('CIE1', 4, 'DC:B4:D9:8B:9E:AC', 'ultrasonic', 
    (SELECT id FROM elementos WHERE alias = 'CIE1'), 
    'topo', 25, TRUE),

('CIE2', 5, 'AA:BB:CC:DD:IE:02', 'ultrasonic', 
    (SELECT id FROM elementos WHERE alias = 'CIE2'), 
    'topo', 25, TRUE),

-- Arduino Nano - RCAV2 (redundância)
('RCAV2', 10, 'DE:AD:BE:EF:FE:ED', 'ultrasonic', 
    (SELECT id FROM elementos WHERE alias = 'RCAV'),  -- Mesmo elemento que node 2
    'lateral', 20, TRUE);

-- Conexões (Fluxo Real do Sistema)
-- Concessionária → IF1/IF2 → RCIF → IE1/IE2 → CB3 → RCON/RCAV

INSERT INTO conexoes (elemento_origem_id, elemento_destino_id, porta_origem, porta_destino, tipo_conexao, diametro_polegadas, material, ativo, observacoes) VALUES
-- Ilha Flores: CIF1/CIF2 → RCIF
((SELECT id FROM elementos WHERE alias = 'CIF1'),
 (SELECT id FROM elementos WHERE alias = 'RCIF'),
 1, 1, 'flange', 2.0, 'PVC', TRUE, 'Alimentação RCIF via bomba IF'),

((SELECT id FROM elementos WHERE alias = 'CIF2'),
 (SELECT id FROM elementos WHERE alias = 'RCIF'),
 1, 1, 'flange', 2.0, 'PVC', TRUE, 'Alimentação RCIF via bomba IF'),

-- RCIF → Cisternas Ilha Engenho
((SELECT id FROM elementos WHERE alias = 'RCIF'),
 (SELECT id FROM elementos WHERE alias = 'CIE1'),
 1, 1, 'flange', 2.5, 'PVC', TRUE, 'Recalque para IE1'),

((SELECT id FROM elementos WHERE alias = 'RCIF'),
 (SELECT id FROM elementos WHERE alias = 'CIE2'),
 1, 1, 'flange', 2.5, 'PVC', TRUE, 'Recalque para IE2'),

-- Cisternas IE → Casa Bombas N03
((SELECT id FROM elementos WHERE alias = 'CIE1'),
 (SELECT id FROM elementos WHERE alias = 'RCB3'),
 1, 1, 'flange', 3.0, 'PVC', TRUE, 'Entrada RCB3 via bomba CB3'),

((SELECT id FROM elementos WHERE alias = 'CIE2'),
 (SELECT id FROM elementos WHERE alias = 'RCB3'),
 1, 1, 'flange', 3.0, 'PVC', TRUE, 'Entrada RCB3 via bomba CB3'),

-- RCB3 → Bombas CB3
((SELECT id FROM elementos WHERE alias = 'RCB3'),
 (SELECT id FROM elementos WHERE alias = 'BOR_CB3_MD1'),
 1, 1, 'flange', 3.0, 'ferro galvanizado', TRUE, 'Entrada bomba diesel'),

((SELECT id FROM elementos WHERE alias = 'RCB3'),
 (SELECT id FROM elementos WHERE alias = 'BOR_CB3_ME1'),
 1, 1, 'flange', 3.0, 'ferro galvanizado', TRUE, 'Entrada bomba elétrica'),

-- Bombas → Válvula Y
((SELECT id FROM elementos WHERE alias = 'BOR_CB3_MD1'),
 (SELECT id FROM elementos WHERE alias = 'VALVY_RCB3_OUT1'),
 1, 1, 'flange', 3.0, 'ferro galvanizado', TRUE, 'Saída bomba diesel'),

((SELECT id FROM elementos WHERE alias = 'BOR_CB3_ME1'),
 (SELECT id FROM elementos WHERE alias = 'VALVY_RCB3_OUT1'),
 1, 1, 'flange', 3.0, 'ferro galvanizado', TRUE, 'Saída bomba elétrica'),

-- Válvula Y → RCON (saída 1)
((SELECT id FROM elementos WHERE alias = 'VALVY_RCB3_OUT1'),
 (SELECT id FROM elementos WHERE alias = 'RCON'),
 1, 1, 'flange', 2.5, 'PVC', TRUE, 'Ramal consumo'),

-- Válvula Y → RCAV (saída 2)
((SELECT id FROM elementos WHERE alias = 'VALVY_RCB3_OUT1'),
 (SELECT id FROM elementos WHERE alias = 'RCAV'),
 2, 1, 'flange', 2.5, 'ferro galvanizado', TRUE, 'Ramal incêndio');
