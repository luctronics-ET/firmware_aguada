<?php
require_once __DIR__ . '/config.php';
$mysqli = db_connect();

$result = $mysqli->query('SELECT * FROM leituras_v2 ORDER BY created_at DESC LIMIT 30');
$rows = $result ? $result->fetch_all(MYSQLI_ASSOC) : [];
?><!doctype html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <title>Aguada - Leituras</title>
  <meta http-equiv="refresh" content="15">
  <style>
    body { font-family: Arial, sans-serif; margin: 24px; background: #f6f8fb; color: #111; }
    h1 { margin: 0 0 12px; }
    table { border-collapse: collapse; width: 100%; background: #fff; }
    th, td { border: 1px solid #ddd; padding: 8px; text-align: center; }
    th { background: #0a6cff; color: #fff; }
    tr:nth-child(even) { background: #f2f7ff; }
    .small { font-size: 12px; color: #444; }
  </style>
</head>
<body>
  <h1>Aguada - Leituras recentes</h1>
  <div class="small">Auto refresh a cada 15s</div>
  <table>
    <tr>
      <th>created_at</th>
      <th>node_id</th>
      <th>mac</th>
      <th>seq</th>
      <th>distance_cm</th>
      <th>level_cm</th>
      <th>%</th>
      <th>volume_l</th>
      <th>vin_mv</th>
      <th>rssi</th>
      <th>ts_ms</th>
    </tr>
    <?php foreach ($rows as $r): ?>
      <tr>
        <td><?php echo htmlspecialchars($r['created_at']); ?></td>
        <td><?php echo (int)$r['node_id']; ?></td>
        <td><?php echo htmlspecialchars($r['mac']); ?></td>
        <td><?php echo (int)$r['seq']; ?></td>
        <td><?php echo (int)$r['distance_cm']; ?></td>
        <td><?php echo (int)$r['level_cm']; ?></td>
        <td><?php echo (int)$r['percentual']; ?></td>
        <td><?php echo (int)$r['volume_l']; ?></td>
        <td><?php echo (int)$r['vin_mv']; ?></td>
        <td><?php echo (int)$r['rssi']; ?></td>
        <td><?php echo (int)$r['ts_ms']; ?></td>
      </tr>
    <?php endforeach; ?>
  </table>
</body>
</html>
