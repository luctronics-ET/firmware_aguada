<?php
// API para fornecer dados ao sistema SCADA
require_once __DIR__ . '/../config.php';

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

$mysqli = new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);
if ($mysqli->connect_error) {
    die(json_encode(['error' => 'Database connection failed']));
}

$mysqli->set_charset('utf8mb4');

// Obter ação
$action = $_GET['action'] ?? 'get_all';

switch ($action) {
    case 'get_all':
        $data = [
            'locais' => getLocais($mysqli),
            'elementos' => getElementos($mysqli),
            'conexoes' => getConexoes($mysqli),
            'anomalias' => getAnomalias($mysqli),
            'tipos_elemento' => getTiposElemento($mysqli)
        ];
        echo json_encode($data);
        break;
        
    case 'toggle_valvula':
        $elemento_id = $_POST['elemento_id'] ?? null;
        $novo_estado = $_POST['estado'] ?? 'fechada';
        $usuario_id = $_POST['usuario_id'] ?? null;
        $motivo = $_POST['motivo'] ?? 'Operação manual';
        
        if ($elemento_id) {
            toggleValvula($mysqli, $elemento_id, $novo_estado, $usuario_id, $motivo);
            echo json_encode(['success' => true, 'message' => 'Válvula alterada']);
        } else {
            echo json_encode(['success' => false, 'error' => 'elemento_id required']);
        }
        break;
        
    case 'toggle_bomba':
        $elemento_id = $_POST['elemento_id'] ?? null;
        $novo_estado = $_POST['estado'] ?? 'desligada';
        $usuario_id = $_POST['usuario_id'] ?? null;
        $motivo = $_POST['motivo'] ?? 'Operação manual';
        
        if ($elemento_id) {
            toggleBomba($mysqli, $elemento_id, $novo_estado, $usuario_id, $motivo);
            echo json_encode(['success' => true, 'message' => 'Bomba alterada']);
        } else {
            echo json_encode(['success' => false, 'error' => 'elemento_id required']);
        }
        break;
        
    case 'get_history':
        $elemento_id = $_GET['elemento_id'] ?? null;
        $hours = $_GET['hours'] ?? 24;
        
        if ($elemento_id) {
            $history = getElementHistory($mysqli, $elemento_id, $hours);
            echo json_encode($history);
        } else {
            echo json_encode(['error' => 'elemento_id required']);
        }
        break;
        
    default:
        echo json_encode(['error' => 'Invalid action']);
}

$mysqli->close();

// ==================== FUNÇÕES ====================

function getLocais($mysqli) {
    $sql = "
        SELECT 
            l.id,
            l.nome,
            l.descricao,
            l.latitude,
            l.longitude,
            COUNT(DISTINCT e.id) as elementos_count,
            COUNT(DISTINCT s.id) as sensores_total,
            SUM(CASE WHEN s.ativo = 1 THEN 1 ELSE 0 END) as sensores_online,
            SUM(CASE WHEN a.severidade = 'critico' AND a.resolvido = FALSE THEN 1 ELSE 0 END) as anomalias_criticas,
            SUM(CASE WHEN a.severidade = 'aviso' AND a.resolvido = FALSE THEN 1 ELSE 0 END) as anomalias_avisos
        FROM locais l
        LEFT JOIN elementos e ON l.id = e.local_id
        LEFT JOIN sensores s ON e.id = s.elemento_id
        LEFT JOIN anomalias a ON e.id = a.elemento_id
        GROUP BY l.id
        ORDER BY l.nome
    ";
    
    $result = $mysqli->query($sql);
    $locais = [];
    while ($row = $result->fetch_assoc()) {
        $locais[] = $row;
    }
    return $locais;
}

function getElementos($mysqli) {
    $sql = "SELECT * FROM vw_status_elementos ORDER BY local_nome, elemento_nome";
    
    $result = $mysqli->query($sql);
    $elementos = [];
    while ($row = $result->fetch_assoc()) {
        // Adicionar posições aleatórias para demo (TODO: salvar no banco)
        $row['pos_x'] = rand(100, 800);
        $row['pos_y'] = rand(100, 500);
        $elementos[] = $row;
    }
    return $elementos;
}

function getConexoes($mysqli) {
    $sql = "SELECT * FROM vw_mapa_sistema";
    
    $result = $mysqli->query($sql);
    $conexoes = [];
    while ($row = $result->fetch_assoc()) {
        $conexoes[] = $row;
    }
    return $conexoes;
}

function getAnomalias($mysqli) {
    $sql = "SELECT * FROM vw_anomalias_ativas ORDER BY severidade DESC, timestamp_inicio DESC";
    
    $result = $mysqli->query($sql);
    $anomalias = [];
    while ($row = $result->fetch_assoc()) {
        $anomalias[] = $row;
    }
    return $anomalias;
}

function getTiposElemento($mysqli) {
    $sql = "SELECT * FROM tipos_elemento ORDER BY categoria, nome";
    
    $result = $mysqli->query($sql);
    $tipos = [];
    while ($row = $result->fetch_assoc()) {
        $tipos[] = $row;
    }
    return $tipos;
}

function toggleValvula($mysqli, $elemento_id, $novo_estado, $usuario_id, $motivo) {
    $stmt = $mysqli->prepare("
        INSERT INTO estados_valvula (elemento_id, estado, percentual_abertura, modo, usuario_id, motivo)
        VALUES (?, ?, ?, 'manual', ?, ?)
    ");
    
    $percentual = ($novo_estado === 'aberta') ? 100 : 0;
    $stmt->bind_param("issis", $elemento_id, $novo_estado, $percentual, $usuario_id, $motivo);
    $stmt->execute();
}

function toggleBomba($mysqli, $elemento_id, $novo_estado, $usuario_id, $motivo) {
    $stmt = $mysqli->prepare("
        INSERT INTO estados_bomba (elemento_id, estado, modo, usuario_id, motivo)
        VALUES (?, ?, 'manual', ?, ?)
    ");
    
    $stmt->bind_param("isis", $elemento_id, $novo_estado, $usuario_id, $motivo);
    $stmt->execute();
}

function getElementHistory($mysqli, $elemento_id, $hours) {
    // Obter tipo do elemento
    $tipo_query = "SELECT tipo_id FROM elementos WHERE id = ?";
    $stmt = $mysqli->prepare($tipo_query);
    $stmt->bind_param("i", $elemento_id);
    $stmt->execute();
    $tipo_result = $stmt->get_result();
    $tipo_row = $tipo_result->fetch_assoc();
    $tipo_id = $tipo_row['tipo_id'];
    
    // Histórico de leituras de sensor
    $sensor_sql = "
        SELECT 
            l.created_at as timestamp,
            l.distance_cm,
            l.level_cm,
            l.percentual,
            l.volume_l,
            l.vin_mv
        FROM leituras_v2 l
        INNER JOIN sensores s ON l.node_id = s.node_id
        WHERE s.elemento_id = ?
        AND l.created_at >= DATE_SUB(NOW(), INTERVAL ? HOUR)
        ORDER BY l.created_at DESC
        LIMIT 1000
    ";
    
    $stmt = $mysqli->prepare($sensor_sql);
    $stmt->bind_param("ii", $elemento_id, $hours);
    $stmt->execute();
    $result = $stmt->get_result();
    
    $history = [
        'leituras' => [],
        'estados_valvula' => [],
        'estados_bomba' => []
    ];
    
    while ($row = $result->fetch_assoc()) {
        $history['leituras'][] = $row;
    }
    
    // Histórico de válvulas
    $valvula_sql = "
        SELECT timestamp, estado, percentual_abertura, modo, motivo
        FROM estados_valvula
        WHERE elemento_id = ?
        AND timestamp >= DATE_SUB(NOW(), INTERVAL ? HOUR)
        ORDER BY timestamp DESC
        LIMIT 100
    ";
    
    $stmt = $mysqli->prepare($valvula_sql);
    $stmt->bind_param("ii", $elemento_id, $hours);
    $stmt->execute();
    $result = $stmt->get_result();
    
    while ($row = $result->fetch_assoc()) {
        $history['estados_valvula'][] = $row;
    }
    
    // Histórico de bombas
    $bomba_sql = "
        SELECT timestamp, estado, frequencia_hz, corrente_a, pressao_bar, vazao_lh, modo, motivo
        FROM estados_bomba
        WHERE elemento_id = ?
        AND timestamp >= DATE_SUB(NOW(), INTERVAL ? HOUR)
        ORDER BY timestamp DESC
        LIMIT 100
    ";
    
    $stmt = $mysqli->prepare($bomba_sql);
    $stmt->bind_param("ii", $elemento_id, $hours);
    $stmt->execute();
    $result = $stmt->get_result();
    
    while ($row = $result->fetch_assoc()) {
        $history['estados_bomba'][] = $row;
    }
    
    return $history;
}
?>
