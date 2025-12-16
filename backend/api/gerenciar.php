<?php
// API para gerenciar locais e elementos (criar, editar, deletar)
require_once '../config.php';

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE');

$mysqli = new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);
if ($mysqli->connect_error) {
    die(json_encode(['success' => false, 'error' => 'Database connection failed']));
}

$mysqli->set_charset('utf8mb4');

$method = $_SERVER['REQUEST_METHOD'];
$action = $_GET['action'] ?? '';

switch ($method) {
    case 'GET':
        handleGet($mysqli, $action);
        break;
    case 'POST':
        handlePost($mysqli, $action);
        break;
    case 'PUT':
        handlePut($mysqli, $action);
        break;
    case 'DELETE':
        handleDelete($mysqli, $action);
        break;
    default:
        echo json_encode(['success' => false, 'error' => 'Method not allowed']);
}

$mysqli->close();

// ==================== GET ====================

function handleGet($mysqli, $action) {
    switch ($action) {
        case 'tipos_elemento':
            getTiposElemento($mysqli);
            break;
        case 'locais':
            getLocais($mysqli);
            break;
        case 'redes_agua':
            getRedesAgua($mysqli);
            break;
        default:
            echo json_encode(['success' => false, 'error' => 'Invalid action']);
    }
}

function getTiposElemento($mysqli) {
    $sql = "SELECT * FROM tipos_elemento ORDER BY categoria, nome";
    $result = $mysqli->query($sql);
    $tipos = [];
    while ($row = $result->fetch_assoc()) {
        $tipos[] = $row;
    }
    echo json_encode(['success' => true, 'data' => $tipos]);
}

function getLocais($mysqli) {
    $sql = "SELECT * FROM locais ORDER BY nome";
    $result = $mysqli->query($sql);
    $locais = [];
    while ($row = $result->fetch_assoc()) {
        $locais[] = $row;
    }
    echo json_encode(['success' => true, 'data' => $locais]);
}

function getRedesAgua($mysqli) {
    $sql = "SELECT * FROM redes_agua WHERE ativo = TRUE ORDER BY nome";
    $result = $mysqli->query($sql);
    $redes = [];
    while ($row = $result->fetch_assoc()) {
        $redes[] = $row;
    }
    echo json_encode(['success' => true, 'data' => $redes]);
}

// ==================== POST (Criar) ====================

function handlePost($mysqli, $action) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    switch ($action) {
        case 'local':
            criarLocal($mysqli, $data);
            break;
        case 'elemento':
            criarElemento($mysqli, $data);
            break;
        case 'sensor':
            criarSensor($mysqli, $data);
            break;
        default:
            echo json_encode(['success' => false, 'error' => 'Invalid action']);
    }
}

function criarLocal($mysqli, $data) {
    $nome = $mysqli->real_escape_string($data['nome']);
    $descricao = $mysqli->real_escape_string($data['descricao'] ?? '');
    $latitude = $data['latitude'] ?? null;
    $longitude = $data['longitude'] ?? null;
    $altitude_m = $data['altitude_m'] ?? null;
    
    $sql = "INSERT INTO locais (nome, descricao, latitude, longitude, altitude_m) 
            VALUES ('$nome', '$descricao', " . 
            ($latitude ? "'$latitude'" : "NULL") . ", " .
            ($longitude ? "'$longitude'" : "NULL") . ", " .
            ($altitude_m ? "'$altitude_m'" : "NULL") . ")";
    
    if ($mysqli->query($sql)) {
        echo json_encode([
            'success' => true, 
            'message' => 'Local criado com sucesso',
            'id' => $mysqli->insert_id
        ]);
    } else {
        echo json_encode([
            'success' => false, 
            'error' => $mysqli->error
        ]);
    }
}

function criarElemento($mysqli, $data) {
    // Validar campos obrigatórios
    if (!isset($data['alias']) || !isset($data['nome']) || !isset($data['tipo_id']) || !isset($data['local_id'])) {
        echo json_encode(['success' => false, 'error' => 'Campos obrigatórios faltando']);
        return;
    }
    
    $alias = $mysqli->real_escape_string($data['alias']);
    $nome = $mysqli->real_escape_string($data['nome']);
    $tipo_id = intval($data['tipo_id']);
    $local_id = intval($data['local_id']);
    $rede_agua_id = isset($data['rede_agua_id']) ? intval($data['rede_agua_id']) : null;
    
    // Campos opcionais
    $capacidade_l = isset($data['capacidade_l']) ? intval($data['capacidade_l']) : null;
    $altura_m = isset($data['altura_m']) ? floatval($data['altura_m']) : null;
    $diametro_cm = isset($data['diametro_cm']) ? floatval($data['diametro_cm']) : null;
    $comprimento_m = isset($data['comprimento_m']) ? floatval($data['comprimento_m']) : null;
    $diametro_polegadas = isset($data['diametro_polegadas']) ? floatval($data['diametro_polegadas']) : null;
    $fabricante = isset($data['fabricante']) ? "'" . $mysqli->real_escape_string($data['fabricante']) . "'" : "NULL";
    $modelo = isset($data['modelo']) ? "'" . $mysqli->real_escape_string($data['modelo']) . "'" : "NULL";
    $ano_instalacao = isset($data['ano_instalacao']) ? intval($data['ano_instalacao']) : null;
    $observacoes = isset($data['observacoes']) ? "'" . $mysqli->real_escape_string($data['observacoes']) . "'" : "NULL";
    
    $sql = "INSERT INTO elementos (
                alias, nome, tipo_id, local_id, rede_agua_id,
                capacidade_l, altura_m, diametro_cm, comprimento_m, diametro_polegadas,
                fabricante, modelo, ano_instalacao, observacoes
            ) VALUES (
                '$alias', '$nome', $tipo_id, $local_id, " . ($rede_agua_id ? $rede_agua_id : "NULL") . ",
                " . ($capacidade_l ? $capacidade_l : "NULL") . ",
                " . ($altura_m ? $altura_m : "NULL") . ",
                " . ($diametro_cm ? $diametro_cm : "NULL") . ",
                " . ($comprimento_m ? $comprimento_m : "NULL") . ",
                " . ($diametro_polegadas ? $diametro_polegadas : "NULL") . ",
                $fabricante, $modelo, " . ($ano_instalacao ? $ano_instalacao : "NULL") . ", $observacoes
            )";
    
    if ($mysqli->query($sql)) {
        echo json_encode([
            'success' => true, 
            'message' => 'Elemento criado com sucesso',
            'id' => $mysqli->insert_id
        ]);
    } else {
        echo json_encode([
            'success' => false, 
            'error' => $mysqli->error
        ]);
    }
}

function criarSensor($mysqli, $data) {
    if (!isset($data['alias']) || !isset($data['node_id']) || !isset($data['mac']) || !isset($data['tipo_sensor'])) {
        echo json_encode(['success' => false, 'error' => 'Campos obrigatórios faltando']);
        return;
    }
    
    $alias = $mysqli->real_escape_string($data['alias']);
    $node_id = intval($data['node_id']);
    $mac = $mysqli->real_escape_string($data['mac']);
    $tipo_sensor = $mysqli->real_escape_string($data['tipo_sensor']);
    $elemento_id = isset($data['elemento_id']) ? intval($data['elemento_id']) : null;
    $posicao_sensor = isset($data['posicao_sensor']) ? "'" . $mysqli->real_escape_string($data['posicao_sensor']) . "'" : "NULL";
    $offset_cm = isset($data['offset_cm']) ? intval($data['offset_cm']) : 0;
    $intervalo_leitura_s = isset($data['intervalo_leitura_s']) ? intval($data['intervalo_leitura_s']) : 30;
    
    $sql = "INSERT INTO sensores (
                alias, node_id, mac, tipo_sensor, elemento_id, posicao_sensor, offset_cm, intervalo_leitura_s
            ) VALUES (
                '$alias', $node_id, '$mac', '$tipo_sensor', " . ($elemento_id ? $elemento_id : "NULL") . ",
                $posicao_sensor, $offset_cm, $intervalo_leitura_s
            )";
    
    if ($mysqli->query($sql)) {
        echo json_encode([
            'success' => true, 
            'message' => 'Sensor criado com sucesso',
            'id' => $mysqli->insert_id
        ]);
    } else {
        echo json_encode([
            'success' => false, 
            'error' => $mysqli->error
        ]);
    }
}

// ==================== PUT (Atualizar) ====================

function handlePut($mysqli, $action) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    switch ($action) {
        case 'local':
            atualizarLocal($mysqli, $data);
            break;
        case 'elemento':
            atualizarElemento($mysqli, $data);
            break;
        default:
            echo json_encode(['success' => false, 'error' => 'Invalid action']);
    }
}

function atualizarLocal($mysqli, $data) {
    if (!isset($data['id'])) {
        echo json_encode(['success' => false, 'error' => 'ID do local é obrigatório']);
        return;
    }
    
    $id = intval($data['id']);
    $updates = [];
    
    if (isset($data['nome'])) {
        $updates[] = "nome = '" . $mysqli->real_escape_string($data['nome']) . "'";
    }
    if (isset($data['descricao'])) {
        $updates[] = "descricao = '" . $mysqli->real_escape_string($data['descricao']) . "'";
    }
    if (isset($data['latitude'])) {
        $updates[] = "latitude = " . floatval($data['latitude']);
    }
    if (isset($data['longitude'])) {
        $updates[] = "longitude = " . floatval($data['longitude']);
    }
    if (isset($data['altitude_m'])) {
        $updates[] = "altitude_m = " . floatval($data['altitude_m']);
    }
    
    if (empty($updates)) {
        echo json_encode(['success' => false, 'error' => 'Nenhum campo para atualizar']);
        return;
    }
    
    $sql = "UPDATE locais SET " . implode(', ', $updates) . " WHERE id = $id";
    
    if ($mysqli->query($sql)) {
        echo json_encode(['success' => true, 'message' => 'Local atualizado com sucesso']);
    } else {
        echo json_encode(['success' => false, 'error' => $mysqli->error]);
    }
}

function atualizarElemento($mysqli, $data) {
    if (!isset($data['id'])) {
        echo json_encode(['success' => false, 'error' => 'ID do elemento é obrigatório']);
        return;
    }
    
    $id = intval($data['id']);
    $updates = [];
    
    $campos = ['alias', 'nome', 'fabricante', 'modelo', 'observacoes'];
    foreach ($campos as $campo) {
        if (isset($data[$campo])) {
            $updates[] = "$campo = '" . $mysqli->real_escape_string($data[$campo]) . "'";
        }
    }
    
    $campos_numericos = ['tipo_id', 'local_id', 'rede_agua_id', 'capacidade_l', 'ano_instalacao'];
    foreach ($campos_numericos as $campo) {
        if (isset($data[$campo])) {
            $updates[] = "$campo = " . intval($data[$campo]);
        }
    }
    
    $campos_decimais = ['altura_m', 'diametro_cm', 'comprimento_m', 'diametro_polegadas'];
    foreach ($campos_decimais as $campo) {
        if (isset($data[$campo])) {
            $updates[] = "$campo = " . floatval($data[$campo]);
        }
    }
    
    if (isset($data['estado_operacional'])) {
        $updates[] = "estado_operacional = '" . $mysqli->real_escape_string($data['estado_operacional']) . "'";
    }
    
    if (empty($updates)) {
        echo json_encode(['success' => false, 'error' => 'Nenhum campo para atualizar']);
        return;
    }
    
    $sql = "UPDATE elementos SET " . implode(', ', $updates) . " WHERE id = $id";
    
    if ($mysqli->query($sql)) {
        echo json_encode(['success' => true, 'message' => 'Elemento atualizado com sucesso']);
    } else {
        echo json_encode(['success' => false, 'error' => $mysqli->error]);
    }
}

// ==================== DELETE ====================

function handleDelete($mysqli, $action) {
    $id = isset($_GET['id']) ? intval($_GET['id']) : 0;
    
    if ($id <= 0) {
        echo json_encode(['success' => false, 'error' => 'ID inválido']);
        return;
    }
    
    switch ($action) {
        case 'local':
            deletarLocal($mysqli, $id);
            break;
        case 'elemento':
            deletarElemento($mysqli, $id);
            break;
        default:
            echo json_encode(['success' => false, 'error' => 'Invalid action']);
    }
}

function deletarLocal($mysqli, $id) {
    // Verificar se há elementos associados
    $check = $mysqli->query("SELECT COUNT(*) as count FROM elementos WHERE local_id = $id");
    $row = $check->fetch_assoc();
    
    if ($row['count'] > 0) {
        echo json_encode([
            'success' => false, 
            'error' => 'Não é possível deletar: existem ' . $row['count'] . ' elementos neste local'
        ]);
        return;
    }
    
    $sql = "DELETE FROM locais WHERE id = $id";
    
    if ($mysqli->query($sql)) {
        echo json_encode(['success' => true, 'message' => 'Local deletado com sucesso']);
    } else {
        echo json_encode(['success' => false, 'error' => $mysqli->error]);
    }
}

function deletarElemento($mysqli, $id) {
    // Verificar se há sensores ou conexões associadas
    $check_sensores = $mysqli->query("SELECT COUNT(*) as count FROM sensores WHERE elemento_id = $id");
    $row = $check_sensores->fetch_assoc();
    
    if ($row['count'] > 0) {
        echo json_encode([
            'success' => false, 
            'error' => 'Não é possível deletar: existem ' . $row['count'] . ' sensores associados'
        ]);
        return;
    }
    
    $sql = "DELETE FROM elementos WHERE id = $id";
    
    if ($mysqli->query($sql)) {
        echo json_encode(['success' => true, 'message' => 'Elemento deletado com sucesso']);
    } else {
        echo json_encode(['success' => false, 'error' => $mysqli->error]);
    }
}
?>
