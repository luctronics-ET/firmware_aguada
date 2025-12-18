<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../config.php';

$conn = db_connect();

// Buscar últimas leituras de cada node (usando MAX(id) para desempate)
$query = "
    SELECT 
        node_id,
        distance_cm,
        level_cm,
        percentual,
        volume_l,
        vin_mv,
        rssi,
        created_at as last_update
    FROM leituras_v2
    WHERE id IN (
        SELECT MAX(id)
        FROM leituras_v2
        WHERE (node_id, created_at) IN (
            SELECT node_id, MAX(created_at)
            FROM leituras_v2
            GROUP BY node_id
        )
        GROUP BY node_id
    )
    ORDER BY node_id
";

$result = $conn->query($query);

// Mapeamento de nodes para nomes reais (atualizado conforme firmware_rules_BASE64.txt)
$node_names = [
    1 => 'RCON',      // Reservatório Consumo
    2 => 'RCAV',      // Reservatório Combate a Incêndio (Avenida)
    3 => 'RCB3',      // Reservatório Casa de Bombas 03
    4 => 'CIE1',      // Castelo Incêndio Elevado - Sensor 1
    5 => 'CIE2',      // Castelo Incêndio Elevado - Sensor 2
    10 => 'RCON-ETH'  // RCON Backup (Nano Ethernet)
];

$node_capacities = [
    1 => 80000,   // RCON: 80m³ = 80.000 litros
    2 => 80000,   // RCAV: 80m³ = 80.000 litros
    3 => 80000,   // RCB3: 80m³ = 80.000 litros
    4 => 245000,  // CIE: 245m³ = 245.000 litros
    5 => 245000,  // CIE: 245m³ = 245.000 litros
    10 => 80000   // RCON Backup: 80m³ = 80.000 litros
];

$node_macs = [
    1 => '20:6E:F1:6B:77:58',
    2 => 'DC:06:75:67:6A:CC',
    3 => '80:F1:B2:50:31:34',
    4 => 'DC:B4:D9:8B:9E:AC',
    5 => 'DC:B4:D9:8B:9E:AC',  // Mesmo MAC (dual sensor)
    10 => 'AA:BB:CC:DD:EE:01'
];

$sensors = [];

if ($result && $result->num_rows > 0) {
    while ($row = $result->fetch_assoc()) {
        $node_id = (int)$row['node_id'];
        $level_percent = (int)$row['percentual'];
        $capacity = $node_capacities[$node_id] ?? 100000;
        
        // Determinar status baseado no nível
        $status = 'normal';
        if ($level_percent < 20) {
            $status = 'alert';
        } elseif ($level_percent < 40) {
            $status = 'warning';
        }
        
        // Calcular bateria em volts (de mV)
        $battery_v = round($row['vin_mv'] / 1000, 2);
        
        $sensors[] = [
            'id' => 'NODE' . $node_id,
            'node_id' => $node_id,
            'name' => $node_names[$node_id] ?? 'Sensor ' . $node_id,
            'mac' => $node_macs[$node_id] ?? 'Unknown',
            'level' => $level_percent,
            'percentual' => $level_percent,
            'volume' => (int)$row['volume_l'],
            'volume_l' => (int)$row['volume_l'],
            'capacity' => $capacity,
            'distance_cm' => (int)$row['distance_cm'],
            'level_cm' => (int)$row['level_cm'],
            'battery' => (int)$row['vin_mv'],
            'battery_v' => $battery_v,
            'vin_mv' => (int)$row['vin_mv'],
            'rssi' => (int)$row['rssi'],
            'status' => $status,
            'last_update' => $row['last_update'],
            'lastUpdate' => $row['last_update'],
            'valve_in' => $level_percent < 80,  // Simulado
            'valve_out' => $level_percent > 20, // Simulado
            'flow' => true,
            'flags' => 0,      // TODO: Adicionar quando firmware enviar
            'alert_type' => 0  // TODO: Adicionar quando firmware enviar
        ];
    }
}

// Se não houver dados, retornar mock básico
if (empty($sensors)) {
    $sensors = [
        [
            'id' => 'NODE1',
            'node_id' => 1,
            'name' => 'Reservatório Ultra 01',
            'level' => 0,
            'volume' => 0,
            'capacity' => 80000,
            'distance_cm' => 0,
            'level_cm' => 0,
            'battery' => 0,
            'battery_v' => 0,
            'rssi' => 0,
            'status' => 'offline',
            'lastUpdate' => date('Y-m-d H:i:s'),
            'valve_in' => false,
            'valve_out' => false,
            'flow' => false
        ]
    ];
}

echo json_encode([
    'success' => true,
    'timestamp' => date('Y-m-d H:i:s'),
    'count' => count($sensors),
    'sensors' => $sensors
], JSON_PRETTY_PRINT);

$conn->close();
