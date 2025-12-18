<?php
require_once __DIR__ . '/config.php';
$mysqli = db_connect();

// Sensor mapping
$sensors = [
    1 => ['alias' => 'CON', 'desc' => 'Consumo'],
    2 => ['alias' => 'RCAV', 'desc' => 'Cavalete'],
    3 => ['alias' => 'RCON', 'desc' => 'Consumo Int.'],
    4 => ['alias' => 'RABD', 'desc' => 'Abdominal'],
    5 => ['alias' => 'RBOM', 'desc' => 'Bomba'],
    6 => ['alias' => 'RAUX', 'desc' => 'Auxiliar'],
];

$result = $mysqli->query('SELECT * FROM leituras_v2 ORDER BY created_at DESC LIMIT 30');
$rows = $result ? $result->fetch_all(MYSQLI_ASSOC) : [];

function signal_quality($rssi) {
    if ($rssi > -60) return ['quality' => '🟢 Excelente', 'color' => '#10b981'];
    if ($rssi > -70) return ['quality' => '🟡 Bom', 'color' => '#f59e0b'];
    if ($rssi > -80) return ['quality' => '🟠 Fraco', 'color' => '#ef4444'];
    return ['quality' => '🔴 Muito fraco', 'color' => '#991b1b'];
}

function battery_status($mv) {
    if ($mv > 4100) return '🔋 OK';
    if ($mv > 3800) return '⚠️ Baixa';
    return '❌ Crítica';
}
?><!doctype html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <title>Aguada - Dashboard</title>
  <meta http-equiv="refresh" content="15">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
      padding: 20px; 
      min-height: 100vh; 
    }
    .container { max-width: 1400px; margin: 0 auto; }
    h1 { color: white; margin-bottom: 20px; font-size: 28px; }
    h2 { color: white; margin: 30px 0 15px; font-size: 18px; }
    .controls { display: flex; gap: 10px; margin-bottom: 20px; flex-wrap: wrap; }
    a { 
      display: inline-block; 
      padding: 10px 20px; 
      background: white; 
      color: #667eea; 
      text-decoration: none; 
      border-radius: 6px; 
      font-weight: bold; 
      transition: 0.3s; 
    }
    a:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.2); }
    .summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-bottom: 20px; }
    .card { 
      background: white; 
      padding: 20px; 
      border-radius: 8px; 
      box-shadow: 0 2px 8px rgba(0,0,0,0.1); 
    }
    .card h3 { color: #667eea; font-size: 12px; text-transform: uppercase; margin-bottom: 10px; }
    .card .value { font-size: 28px; font-weight: bold; color: #111; }
    .card .unit { font-size: 12px; color: #666; margin-top: 5px; }
    .card .status { font-size: 14px; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee; }
    table { 
      border-collapse: collapse; 
      width: 100%; 
      background: white; 
      border-radius: 8px; 
      overflow: hidden; 
      box-shadow: 0 2px 8px rgba(0,0,0,0.1); 
      margin-bottom: 20px; 
    }
    th { 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
      color: white; 
      padding: 15px 10px; 
      text-align: left; 
      font-weight: 600; 
      font-size: 13px; 
      text-transform: uppercase; 
    }
    td { padding: 12px 10px; border-bottom: 1px solid #eee; font-size: 13px; }
    tr:hover { background: #f8f9ff; }
    .node-id { font-weight: bold; color: #667eea; }
    .reservoir { background: #f0f4ff; border-radius: 4px; padding: 6px 10px; font-weight: 600; }
    .small { font-size: 11px; color: #999; }
    .volume-bar { width: 100%; background: #eee; border-radius: 4px; height: 6px; overflow: hidden; }
    .volume-fill { height: 100%; background: linear-gradient(90deg, #667eea, #764ba2); }
    .footer { color: white; font-size: 12px; text-align: center; margin-top: 20px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>📊 Dashboard AGUADA - Telemetria de Água</h1>
    
    <div class="controls">
      <a href="relatorio_servico.html">📈 Balanço Hídrico</a>
      <a href="scada.html">🎛️ SCADA</a>
      <a href="index.php">🏠 Início</a>
    </div>

    <div class="summary">
      <?php 
      $summary = [];
      foreach ($rows as $r) {
        $nid = $r['node_id'];
        if (!isset($summary[$nid])) {
          $summary[$nid] = [];
        }
        $summary[$nid][] = $r;
      }
      
      foreach ($summary as $nid => $readings):
        $latest_r = $readings[0];
        $sensor = $sensors[$nid] ?? ['alias' => "N$nid", 'desc' => 'Sensor'];
        $pct = (int)$latest_r['percentual'];
        $color = $pct > 75 ? '#10b981' : ($pct > 50 ? '#f59e0b' : '#ef4444');
      ?>
      <div class="card">
        <h3>🏊 <?php echo htmlspecialchars($sensor['alias']); ?> - <?php echo htmlspecialchars($sensor['desc']); ?></h3>
        <div class="value" style="color: <?php echo $color; ?>;"><?php echo $pct; ?>%</div>
        <div class="unit"><?php echo (int)$latest_r['volume_l']; ?> L / <?php echo (int)$latest_r['level_cm']; ?> cm</div>
        <div class="volume-bar" style="margin: 10px 0;">
          <div class="volume-fill" style="width: <?php echo $pct; ?>%; background: <?php echo $color; ?>;"></div>
        </div>
        <div class="status">
          <?php 
            $sig = signal_quality((int)$latest_r['rssi']);
            echo '<div>' . $sig['quality'] . ' RSSI: ' . (int)$latest_r['rssi'] . ' dBm</div>';
            echo '<div>' . battery_status((int)$latest_r['vin_mv']) . ' ' . (int)$latest_r['vin_mv'] . ' mV</div>';
          ?>
        </div>
      </div>
      <?php endforeach; ?>
    </div>

    <h2>📋 Histórico de Leituras</h2>
    <table>
      <thead>
        <tr>
          <th>⏱️ Data/Hora</th>
          <th>🏊 Reservatório</th>
          <th>📍 Node</th>
          <th>📏 Nível (cm)</th>
          <th>📊 Volume (L)</th>
          <th>💧 %</th>
          <th>🔋 Bateria</th>
          <th>📡 Sinal</th>
          <th>Seq</th>
        </tr>
      </thead>
      <tbody>
      <?php foreach ($rows as $r):
        $sensor = $sensors[(int)$r['node_id']] ?? ['alias' => 'N' . (int)$r['node_id'], 'desc' => 'N/A'];
        $sig = signal_quality((int)$r['rssi']);
        $bat = battery_status((int)$r['vin_mv']);
        $pct = (int)$r['percentual'];
        $color = $pct > 75 ? '#10b981' : ($pct > 50 ? '#f59e0b' : '#ef4444');
      ?>
      <tr>
        <td class="small"><?php echo htmlspecialchars($r['created_at']); ?></td>
        <td><span class="reservoir"><?php echo htmlspecialchars($sensor['alias']); ?></span></td>
        <td class="node-id"><?php echo (int)$r['node_id']; ?></td>
        <td><?php echo (int)$r['level_cm']; ?> cm</td>
        <td><strong><?php echo number_format((int)$r['volume_l'], 0, ',', '.'); ?></strong> L</td>
        <td style="color: <?php echo $color; ?>; font-weight: bold;"><?php echo $pct; ?>%</td>
        <td><?php echo $bat; ?></td>
        <td><?php echo $sig['quality']; ?></td>
        <td class="small"><?php echo (int)$r['seq']; ?></td>
      </tr>
      <?php endforeach; ?>
      </tbody>
    </table>
    
    <div class="footer">
      🔄 Auto-atualiza a cada 15s | Última atualização: <?php echo date('H:i:s'); ?>
    </div>
  </div>
</body>
</html>
