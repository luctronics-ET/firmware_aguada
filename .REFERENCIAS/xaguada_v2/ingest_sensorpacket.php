<?php
require_once __DIR__ . '/config.php';

header('Content-Type: application/json');

$mysqli = new mysqli(DB_HOST, DB_USER, DB_PASS, DB_NAME);
if ($mysqli->connect_errno) {
    http_response_code(500);
    echo json_encode(['status'=>'error','msg'=>'DB error']);
    exit;
}

// garante tabela
$mysqli->query("CREATE TABLE IF NOT EXISTS leituras_v2 (
  id INT AUTO_INCREMENT PRIMARY KEY,
  ts TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  node_id TINYINT UNSIGNED,
  mac VARCHAR(17),
  seq INT UNSIGNED,
  version TINYINT UNSIGNED,
  distance_cm INT,
  level_cm INT,
  percentual FLOAT,
  volume_l INT,
  vin_mv INT,
  rssi INT,
  ts_ms BIGINT UNSIGNED,
  raw JSON NULL,
  KEY idx_ts (ts),
  KEY idx_node (node_id),
  KEY idx_mac (mac),
  KEY idx_seq (seq)
);");

$body = file_get_contents('php://input');
$data = json_decode($body, true);
if (!$data) {
    http_response_code(400);
    echo json_encode(['status'=>'error','msg'=>'JSON inválido']);
    exit;
}

// extrai campos do SensorPacket-like JSON
$version      = isset($data['version']) ? intval($data['version']) : null;
$node_id      = isset($data['node_id']) ? intval($data['node_id']) : null;
$mac          = isset($data['mac']) ? substr(trim($data['mac']), 0, 17) : null;
$seq          = isset($data['seq']) ? intval($data['seq']) : null;
$distance_cm  = isset($data['distance_cm']) ? intval($data['distance_cm']) : null;
$level_cm     = isset($data['level_cm']) ? intval($data['level_cm']) : null;
$percentual   = isset($data['percentual']) ? floatval($data['percentual']) : null;
$volume_l     = isset($data['volume_l']) ? intval($data['volume_l']) : null;
$vin_mv       = isset($data['vin_mv']) ? intval($data['vin_mv']) : 0;
$rssi         = isset($data['rssi']) ? intval($data['rssi']) : 0;
$ts_ms        = isset($data['ts_ms']) ? intval($data['ts_ms']) : null;

// valida mínimos
if ($node_id === null || $mac === null || $seq === null || $distance_cm === null || $level_cm === null || $percentual === null || $volume_l === null) {
    http_response_code(400);
    echo json_encode(['status'=>'error','msg'=>'campos obrigatórios faltando']);
    exit;
}

$raw_json = json_encode($data);

$stmt = $mysqli->prepare("INSERT INTO leituras_v2 (node_id, mac, seq, version, distance_cm, level_cm, percentual, volume_l, vin_mv, rssi, ts_ms, raw) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
$stmt->bind_param('isiiiidiiiis', $node_id, $mac, $seq, $version, $distance_cm, $level_cm, $percentual, $volume_l, $vin_mv, $rssi, $ts_ms, $raw_json);

if (!$stmt->execute()) {
    http_response_code(500);
    echo json_encode(['status'=>'error','msg'=>'insert fail']);
    $stmt->close();
    $mysqli->close();
    exit;
}
$stmt->close();
$mysqli->close();

echo json_encode(['status'=>'ok']);
?>
