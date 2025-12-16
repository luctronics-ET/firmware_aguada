<?php
/**
 * API de Relatórios de Serviço e Balanço Hídrico
 * Endpoints para criar, listar, validar e calcular balanços
 */

require_once __DIR__ . '/../config.php';

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE');
header('Access-Control-Allow-Headers: Content-Type');

$mysqli = db_connect();

// Obter ação da requisição
$action = $_GET['action'] ?? $_POST['action'] ?? 'list';

try {
    switch ($action) {
        case 'list':
            listarRelatorios($mysqli);
            break;
        
        case 'create':
            criarRelatorio($mysqli);
            break;
        
        case 'get':
            obterRelatorio($mysqli);
            break;
        
        case 'update':
            atualizarRelatorio($mysqli);
            break;
        
        case 'validate':
            validarRelatorio($mysqli);
            break;
        
        case 'delete':
            deletarRelatorio($mysqli);
            break;
        
        case 'calcular_balanco':
            calcularBalanco($mysqli);
            break;
        
        case 'get_balanco_diario':
            obterBalancoDiario($mysqli);
            break;
        
        case 'registrar_abastecimento':
            registrarAbastecimento($mysqli);
            break;
        
        case 'get_pendentes':
            obterPendentes($mysqli);
            break;
        
        default:
            throw new Exception("Ação inválida: $action");
    }
} catch (Exception $e) {
    http_response_code(400);
    echo json_encode([
        'success' => false,
        'error' => $e->getMessage()
    ]);
}

// ============================================================================
// FUNÇÕES DE RELATÓRIOS
// ============================================================================

function listarRelatorios($mysqli) {
    $page = intval($_GET['page'] ?? 1);
    $limit = intval($_GET['limit'] ?? 20);
    $offset = ($page - 1) * $limit;
    
    $filtros = [];
    $params = [];
    $types = '';
    
    // Filtros opcionais
    if (!empty($_GET['data_inicio'])) {
        $filtros[] = "r.data_relatorio >= ?";
        $params[] = $_GET['data_inicio'];
        $types .= 's';
    }
    
    if (!empty($_GET['data_fim'])) {
        $filtros[] = "r.data_relatorio <= ?";
        $params[] = $_GET['data_fim'];
        $types .= 's';
    }
    
    if (!empty($_GET['operador'])) {
        $filtros[] = "r.operador LIKE ?";
        $params[] = '%' . $_GET['operador'] . '%';
        $types .= 's';
    }
    
    if (!empty($_GET['validado'])) {
        $filtros[] = "r.validado = ?";
        $params[] = $_GET['validado'] === 'true' ? 1 : 0;
        $types .= 'i';
    }
    
    $where = count($filtros) > 0 ? 'WHERE ' . implode(' AND ', $filtros) : '';
    
    // Consulta principal
    $sql = "
        SELECT 
            r.*,
            COUNT(rr.id) as num_reservatorios,
            SUM(rr.consumo_litros) as consumo_total,
            SUM(rr.abastecimento_litros) as abastecimento_total
        FROM relatorios_servico r
        LEFT JOIN relatorio_reservatorios rr ON r.id = rr.relatorio_id
        $where
        GROUP BY r.id
        ORDER BY r.data_relatorio DESC, r.criado_em DESC
        LIMIT ? OFFSET ?
    ";
    
    $stmt = $mysqli->prepare($sql);
    
    // Bind params
    $params[] = $limit;
    $params[] = $offset;
    $types .= 'ii';
    
    if (count($params) > 0) {
        $stmt->bind_param($types, ...$params);
    }
    
    $stmt->execute();
    $result = $stmt->get_result();
    
    $relatorios = [];
    while ($row = $result->fetch_assoc()) {
        $relatorios[] = $row;
    }
    
    // Contar total
    $countSql = "SELECT COUNT(*) as total FROM relatorios_servico r $where";
    $countStmt = $mysqli->prepare($countSql);
    
    if (count($filtros) > 0) {
        $countTypes = substr($types, 0, -2); // Remove limit/offset types
        $countParams = array_slice($params, 0, -2); // Remove limit/offset values
        $countStmt->bind_param($countTypes, ...$countParams);
    }
    
    $countStmt->execute();
    $totalResult = $countStmt->get_result();
    $total = $totalResult->fetch_assoc()['total'];
    
    echo json_encode([
        'success' => true,
        'data' => $relatorios,
        'pagination' => [
            'page' => $page,
            'limit' => $limit,
            'total' => intval($total),
            'pages' => ceil($total / $limit)
        ]
    ]);
}

function criarRelatorio($mysqli) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    // Validar campos obrigatórios
    $required = ['data_relatorio', 'turno', 'operador'];
    foreach ($required as $field) {
        if (empty($data[$field])) {
            throw new Exception("Campo obrigatório: $field");
        }
    }
    
    // Iniciar transação
    $mysqli->begin_transaction();
    
    try {
        // Inserir relatório principal
        $stmt = $mysqli->prepare("
            INSERT INTO relatorios_servico (
                data_relatorio, turno, operador, supervisor, status_geral,
                condicoes_climaticas, ocorrencias, manutencoes_realizadas, pendencias
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ");
        
        $stmt->bind_param(
            'sssssssss',
            $data['data_relatorio'],
            $data['turno'],
            $data['operador'],
            $data['supervisor'] ?? null,
            $data['status_geral'] ?? 'NORMAL',
            $data['condicoes_climaticas'] ?? null,
            $data['ocorrencias'] ?? null,
            $data['manutencoes'] ?? null,
            $data['pendencias'] ?? null
        );
        
        $stmt->execute();
        $relatorio_id = $mysqli->insert_id;
        
        // Inserir dados dos reservatórios
        if (!empty($data['reservatorios']) && is_array($data['reservatorios'])) {
            $stmtRes = $mysqli->prepare("
                INSERT INTO relatorio_reservatorios (
                    relatorio_id, reservatorio_id,
                    nivel_inicial_cm, nivel_final_cm,
                    percentual_inicial, percentual_final,
                    volume_inicial_litros, volume_final_litros,
                    consumo_litros, abastecimento_litros,
                    bomba_utilizada, valvula_entrada, valvula_saida,
                    estado_operacional, observacoes, dados_automaticos
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ");
            
            foreach ($data['reservatorios'] as $res) {
                // Calcular consumo se não fornecido
                $consumo = $res['consumo_litros'] ?? 
                    (($res['volume_inicial_litros'] ?? 0) - ($res['volume_final_litros'] ?? 0));
                
                $stmtRes->bind_param(
                    'isiiiiiiissssssi',
                    $relatorio_id,
                    $res['reservatorio_id'],
                    $res['nivel_inicial_cm'] ?? null,
                    $res['nivel_final_cm'] ?? null,
                    $res['percentual_inicial'] ?? null,
                    $res['percentual_final'] ?? null,
                    $res['volume_inicial_litros'] ?? null,
                    $res['volume_final_litros'] ?? null,
                    $consumo,
                    $res['abastecimento_litros'] ?? 0,
                    $res['bomba_utilizada'] ?? null,
                    $res['valvula_entrada'] ?? null,
                    $res['valvula_saida'] ?? null,
                    $res['estado_operacional'] ?? 'NORMAL',
                    $res['observacoes'] ?? null,
                    $res['dados_automaticos'] ?? 0
                );
                
                $stmtRes->execute();
            }
        }
        
        $mysqli->commit();
        
        echo json_encode([
            'success' => true,
            'relatorio_id' => $relatorio_id,
            'message' => 'Relatório criado com sucesso'
        ]);
        
    } catch (Exception $e) {
        $mysqli->rollback();
        throw $e;
    }
}

function obterRelatorio($mysqli) {
    $id = intval($_GET['id'] ?? 0);
    if ($id <= 0) {
        throw new Exception("ID inválido");
    }
    
    // Buscar relatório principal
    $stmt = $mysqli->prepare("SELECT * FROM relatorios_servico WHERE id = ?");
    $stmt->bind_param('i', $id);
    $stmt->execute();
    $result = $stmt->get_result();
    
    $relatorio = $result->fetch_assoc();
    if (!$relatorio) {
        throw new Exception("Relatório não encontrado");
    }
    
    // Buscar dados dos reservatórios
    $stmtRes = $mysqli->prepare("SELECT * FROM relatorio_reservatorios WHERE relatorio_id = ? ORDER BY reservatorio_id");
    $stmtRes->bind_param('i', $id);
    $stmtRes->execute();
    $resultRes = $stmtRes->get_result();
    
    $reservatorios = [];
    while ($row = $resultRes->fetch_assoc()) {
        $reservatorios[] = $row;
    }
    
    $relatorio['reservatorios'] = $reservatorios;
    
    echo json_encode([
        'success' => true,
        'data' => $relatorio
    ]);
}

function validarRelatorio($mysqli) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    $id = intval($data['id'] ?? 0);
    $validado_por = $data['validado_por'] ?? null;
    
    if ($id <= 0) {
        throw new Exception("ID inválido");
    }
    
    if (empty($validado_por)) {
        throw new Exception("Nome do supervisor obrigatório");
    }
    
    $stmt = $mysqli->prepare("
        UPDATE relatorios_servico 
        SET validado = 1, validado_por = ?, validado_em = NOW()
        WHERE id = ?
    ");
    
    $stmt->bind_param('si', $validado_por, $id);
    $stmt->execute();
    
    if ($stmt->affected_rows === 0) {
        throw new Exception("Relatório não encontrado ou já validado");
    }
    
    echo json_encode([
        'success' => true,
        'message' => 'Relatório validado com sucesso'
    ]);
}

// ============================================================================
// FUNÇÕES DE BALANÇO HÍDRICO
// ============================================================================

function calcularBalanco($mysqli) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    $reservatorio = $data['reservatorio'] ?? null;
    $inicio = $data['periodo_inicio'] ?? null;
    $fim = $data['periodo_fim'] ?? null;
    
    if (!$reservatorio || !$inicio || !$fim) {
        throw new Exception("Parâmetros obrigatórios: reservatorio, periodo_inicio, periodo_fim");
    }
    
    // Chamar stored procedure
    $stmt = $mysqli->prepare("CALL calcular_balanco_hidrico(?, ?, ?)");
    $stmt->bind_param('sss', $reservatorio, $inicio, $fim);
    $stmt->execute();
    
    // Buscar resultado calculado
    $stmtResult = $mysqli->prepare("
        SELECT * FROM balanco_hidrico 
        WHERE reservatorio_id = ? AND periodo_inicio = ? AND periodo_fim = ?
    ");
    $stmtResult->bind_param('sss', $reservatorio, $inicio, $fim);
    $stmtResult->execute();
    $result = $stmtResult->get_result();
    
    $balanco = $result->fetch_assoc();
    
    echo json_encode([
        'success' => true,
        'data' => $balanco
    ]);
}

function obterBalancoDiario($mysqli) {
    $data_inicio = $_GET['data_inicio'] ?? date('Y-m-d', strtotime('-7 days'));
    $data_fim = $_GET['data_fim'] ?? date('Y-m-d');
    $reservatorio = $_GET['reservatorio'] ?? null;
    
    $where = "WHERE data BETWEEN ? AND ?";
    $params = [$data_inicio, $data_fim];
    $types = 'ss';
    
    if ($reservatorio) {
        $where .= " AND reservatorio_id = ?";
        $params[] = $reservatorio;
        $types .= 's';
    }
    
    $stmt = $mysqli->prepare("SELECT * FROM vw_balanco_diario $where ORDER BY data DESC, reservatorio_id");
    $stmt->bind_param($types, ...$params);
    $stmt->execute();
    $result = $stmt->get_result();
    
    $balanco = [];
    while ($row = $result->fetch_assoc()) {
        $balanco[] = $row;
    }
    
    echo json_encode([
        'success' => true,
        'data' => $balanco
    ]);
}

function registrarAbastecimento($mysqli) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    $required = ['reservatorio_origem', 'reservatorio_destino', 'volume_litros'];
    foreach ($required as $field) {
        if (empty($data[$field])) {
            throw new Exception("Campo obrigatório: $field");
        }
    }
    
    $stmt = $mysqli->prepare("
        INSERT INTO eventos_abastecimento (
            datetime, reservatorio_origem, reservatorio_destino,
            volume_litros, duracao_minutos, bomba_utilizada, vazao_lpm, operador, observacoes
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    ");
    
    $datetime = $data['datetime'] ?? date('Y-m-d H:i:s');
    $vazao = null;
    if (!empty($data['duracao_minutos']) && $data['volume_litros'] > 0) {
        $vazao = $data['volume_litros'] / $data['duracao_minutos'];
    }
    
    $stmt->bind_param(
        'sssiiidss',
        $datetime,
        $data['reservatorio_origem'],
        $data['reservatorio_destino'],
        $data['volume_litros'],
        $data['duracao_minutos'] ?? null,
        $data['bomba_utilizada'] ?? null,
        $vazao,
        $data['operador'] ?? null,
        $data['observacoes'] ?? null
    );
    
    $stmt->execute();
    
    echo json_encode([
        'success' => true,
        'abastecimento_id' => $mysqli->insert_id,
        'message' => 'Abastecimento registrado com sucesso'
    ]);
}

function obterPendentes($mysqli) {
    $stmt = $mysqli->prepare("SELECT * FROM vw_relatorios_pendentes ORDER BY dias_atraso DESC");
    $stmt->execute();
    $result = $stmt->get_result();
    
    $pendentes = [];
    while ($row = $result->fetch_assoc()) {
        $pendentes[] = $row;
    }
    
    echo json_encode([
        'success' => true,
        'data' => $pendentes,
        'total' => count($pendentes)
    ]);
}

function deletarRelatorio($mysqli) {
    $id = intval($_GET['id'] ?? $_POST['id'] ?? 0);
    if ($id <= 0) {
        throw new Exception("ID inválido");
    }
    
    // Verificar se está validado
    $stmt = $mysqli->prepare("SELECT validado FROM relatorios_servico WHERE id = ?");
    $stmt->bind_param('i', $id);
    $stmt->execute();
    $result = $stmt->get_result();
    $relatorio = $result->fetch_assoc();
    
    if (!$relatorio) {
        throw new Exception("Relatório não encontrado");
    }
    
    if ($relatorio['validado']) {
        throw new Exception("Não é possível deletar relatório validado");
    }
    
    // Deletar (CASCADE deletará relatorio_reservatorios)
    $stmtDel = $mysqli->prepare("DELETE FROM relatorios_servico WHERE id = ?");
    $stmtDel->bind_param('i', $id);
    $stmtDel->execute();
    
    echo json_encode([
        'success' => true,
        'message' => 'Relatório deletado com sucesso'
    ]);
}

function atualizarRelatorio($mysqli) {
    $data = json_decode(file_get_contents('php://input'), true);
    
    $id = intval($data['id'] ?? 0);
    if ($id <= 0) {
        throw new Exception("ID inválido");
    }
    
    // Verificar se está validado
    $stmt = $mysqli->prepare("SELECT validado FROM relatorios_servico WHERE id = ?");
    $stmt->bind_param('i', $id);
    $stmt->execute();
    $result = $stmt->get_result();
    $relatorio = $result->fetch_assoc();
    
    if (!$relatorio) {
        throw new Exception("Relatório não encontrado");
    }
    
    if ($relatorio['validado']) {
        throw new Exception("Não é possível editar relatório validado");
    }
    
    // Atualizar campos permitidos
    $mysqli->begin_transaction();
    
    try {
        $stmtUpd = $mysqli->prepare("
            UPDATE relatorios_servico SET
                status_geral = ?, condicoes_climaticas = ?, ocorrencias = ?,
                manutencoes_realizadas = ?, pendencias = ?
            WHERE id = ?
        ");
        
        $stmtUpd->bind_param(
            'sssssi',
            $data['status_geral'] ?? 'NORMAL',
            $data['condicoes_climaticas'] ?? null,
            $data['ocorrencias'] ?? null,
            $data['manutencoes'] ?? null,
            $data['pendencias'] ?? null,
            $id
        );
        
        $stmtUpd->execute();
        
        $mysqli->commit();
        
        echo json_encode([
            'success' => true,
            'message' => 'Relatório atualizado com sucesso'
        ]);
        
    } catch (Exception $e) {
        $mysqli->rollback();
        throw $e;
    }
}
