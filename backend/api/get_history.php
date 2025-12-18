<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../config.php';

$conn = db_connect();

// Parâmetros
$node_id = isset($_GET['node_id']) ? (int)$_GET['node_id'] : null;
$hours = isset($_GET['hours']) ? (int)$_GET['hours'] : 24;

// Query para histórico
$query = "
    SELECT 
        DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:00') as timestamp,
        node_id,
        AVG(level_cm) as avg_level_cm,
        AVG(percentual) as avg_percentual,
        AVG(volume_l) as avg_volume_l,
        AVG(rssi) as avg_rssi,
        COUNT(*) as reading_count
    FROM leituras_v2
    WHERE created_at >= NOW() - INTERVAL $hours HOUR
";

if ($node_id !== null) {
    $query .= " AND node_id = " . $conn->real_escape_string($node_id);
}

$query .= "
    GROUP BY DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:00'), node_id
    ORDER BY timestamp ASC
";

$result = $conn->query($query);

if (!$result) {
    echo json_encode([
        'status' => 'error',
        'message' => 'Database query failed: ' . $conn->error
    ]);
    exit;
}

$history = [];
while ($row = $result->fetch_assoc()) {
    $history[] = [
        'timestamp' => $row['timestamp'],
        'node_id' => (int)$row['node_id'],
        'avg_level_cm' => round($row['avg_level_cm'], 2),
        'avg_percentual' => round($row['avg_percentual'], 2),
        'avg_volume_l' => round($row['avg_volume_l'], 2),
        'avg_rssi' => round($row['avg_rssi'], 2),
        'reading_count' => (int)$row['reading_count']
    ];
}

echo json_encode([
    'status' => 'success',
    'hours' => $hours,
    'count' => count($history),
    'history' => $history
]);

$conn->close();
?>
