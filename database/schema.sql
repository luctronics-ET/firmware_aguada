-- Tabela para SensorPacketV1 (versao 2)
CREATE TABLE IF NOT EXISTS leituras_v2 (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    version TINYINT NOT NULL,
    node_id SMALLINT NOT NULL,
    mac VARCHAR(17) NOT NULL,
    seq INT NOT NULL,
    distance_cm INT,
    level_cm INT,
    percentual TINYINT,
    volume_l INT,
    vin_mv INT,
    rssi TINYINT,
    ts_ms BIGINT
);

CREATE INDEX idx_leituras_v2_node_ts ON leituras_v2 (node_id, created_at);
