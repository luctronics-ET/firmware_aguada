<?php
// Carregar configuração do banco
require_once 'config.php';

// Conectar ao banco
$conn = new mysqli($hostname, $username, $password, $database);

if ($conn->connect_error) {
    die("Erro de conexão: " . $conn->connect_error);
}

// Consultar leituras de hoje
$query_hoje = "SELECT COUNT(*) as total, MAX(created_at) as ultima 
               FROM leituras_v2 
               WHERE DATE(created_at) = CURDATE()";
$result_hoje = $conn->query($query_hoje);
$row_hoje = $result_hoje->fetch_assoc();

echo "========================================\n";
echo "RELATÓRIO DE LEITURAS - " . date('Y-m-d H:i:s') . "\n";
echo "========================================\n\n";

echo "📊 HOJE (" . date('Y-m-d') . "):\n";
echo "  Total de leituras: " . $row_hoje['total'] . "\n";
echo "  Última leitura: " . ($row_hoje['ultima'] ?? 'Nenhuma') . "\n\n";

// Consultar leituras por sensor hoje
$query_por_node = "SELECT node_id, COUNT(*) as total, 
                   MAX(created_at) as ultima,
                   AVG(distance_cm) as dist_media,
                   AVG(level_cm) as nivel_medio
                   FROM leituras_v2 
                   WHERE DATE(created_at) = CURDATE() 
                   GROUP BY node_id 
                   ORDER BY node_id";
$result_nodes = $conn->query($query_por_node);

echo "📡 LEITURAS POR SENSOR:\n";
if ($result_nodes->num_rows > 0) {
    while($row = $result_nodes->fetch_assoc()) {
        echo "  Node " . $row['node_id'] . ":\n";
        echo "    - Leituras: " . $row['total'] . "\n";
        echo "    - Última: " . $row['ultima'] . "\n";
        echo "    - Dist. média: " . round($row['dist_media']) . " cm\n";
        echo "    - Nível médio: " . round($row['nivel_medio']) . " cm\n\n";
    }
} else {
    echo "  ⚠️ Nenhuma leitura hoje\n\n";
}

// Últimas 5 leituras (qualquer dia)
$query_ultimas = "SELECT node_id, distance_cm, level_cm, percentual, created_at 
                  FROM leituras_v2 
                  ORDER BY created_at DESC 
                  LIMIT 5";
$result_ultimas = $conn->query($query_ultimas);

echo "🕐 ÚLTIMAS 5 LEITURAS (qualquer dia):\n";
if ($result_ultimas->num_rows > 0) {
    while($row = $result_ultimas->fetch_assoc()) {
        echo "  [" . $row['created_at'] . "] ";
        echo "Node " . $row['node_id'] . ": ";
        echo $row['distance_cm'] . " cm, ";
        echo $row['level_cm'] . " cm, ";
        echo $row['percentual'] . "%\n";
    }
} else {
    echo "  ⚠️ Nenhuma leitura no banco\n";
}

echo "\n";

// Total geral de leituras
$query_total = "SELECT COUNT(*) as total FROM leituras_v2";
$result_total = $conn->query($query_total);
$row_total = $result_total->fetch_assoc();

echo "📈 TOTAL GERAL NO BANCO: " . $row_total['total'] . " leituras\n";

$conn->close();
?>
