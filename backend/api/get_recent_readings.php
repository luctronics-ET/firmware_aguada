<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../config.php';

$conn = db_connect();

// Parâmetros
$limit = isset($_GET['limit']) ? (int)$_GET['limit'] : 50;
$node_id = isset($_GET['node_id']) ? (int)$_GET['node_id'] : null;

// Query base
$query = "
    SELECT 
        id,
        created_at,
        node_id,
        mac,
        seq,
        distance_cm,
        level_cm,
        percentual,
        volume_l,
        vin_mv,
        rssi,
        ts_ms
    FROM leituras_v2
";

// Filtrar por node_id se especificado
if ($node_id !== null) {
    $query .= " WHERE node_id = " . $conn->real_escape_string($node_id);
}

$query .= " ORDER BY created_at DESC LIMIT " . $limit;

$result = $conn->query($query);

if (!$result) {
    echo json_encode([
        'status' => 'error',
        'message' => 'Database query failed: ' . $conn->error
    ]);
    exit;
}

$readings = [];
while ($row = $result->fetch_assoc()) {
    $readings[] = $row;
}

echo json_encode([
    'status' => 'success',
    'count' => count($readings),
    'readings' => $readings
]);

$conn->close();
?>
