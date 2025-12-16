<?php
// Recebe JSON de SensorPacketV1 e insere em leituras_v2
require_once __DIR__ . '/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    header('Allow: POST');
    exit('Only POST allowed');
}

$raw = file_get_contents('php://input');
$data = json_decode($raw, true);
if (!$data) {
    http_response_code(400);
    exit('Invalid JSON');
}

$fields = [
    'version' => FILTER_VALIDATE_INT,
    'node_id' => FILTER_VALIDATE_INT,
    'mac' => FILTER_UNSAFE_RAW,
    'seq' => FILTER_VALIDATE_INT,
    'distance_cm' => FILTER_VALIDATE_INT,
    'level_cm' => FILTER_VALIDATE_INT,
    'percentual' => FILTER_VALIDATE_INT,
    'volume_l' => FILTER_VALIDATE_INT,
    'vin_mv' => FILTER_VALIDATE_INT,
    'rssi' => FILTER_VALIDATE_INT,
    'ts_ms' => FILTER_VALIDATE_INT,
];

$clean = [];
foreach ($fields as $key => $filter) {
    if (!array_key_exists($key, $data)) {
        $clean[$key] = null;
        continue;
    }
    $clean[$key] = filter_var($data[$key], $filter);
}

// Validação mínima
if ($clean['version'] === false || $clean['node_id'] === false || $clean['seq'] === false) {
    http_response_code(400);
    exit('Missing required numeric fields');
}

$mysqli = db_connect();
$stmt = $mysqli->prepare('INSERT INTO leituras_v2 (version, node_id, mac, seq, distance_cm, level_cm, percentual, volume_l, vin_mv, rssi, ts_ms) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)');
if (!$stmt) {
    http_response_code(500);
    exit('Prepare failed');
}
$stmt->bind_param(
    'iisiiiiiiii',
    $clean['version'],
    $clean['node_id'],
    $clean['mac'],
    $clean['seq'],
    $clean['distance_cm'],
    $clean['level_cm'],
    $clean['percentual'],
    $clean['volume_l'],
    $clean['vin_mv'],
    $clean['rssi'],
    $clean['ts_ms']
);

if (!$stmt->execute()) {
    http_response_code(500);
    exit('Insert failed');
}

echo 'ok';
