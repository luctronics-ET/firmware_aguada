<?php
require_once __DIR__ . '/config.php';

$mysqli = new mysqli(DB_HOST, DB_USER, DB_PASS, DB_NAME);
if ($mysqli->connect_errno) die('Erro DB: '.$mysqli->connect_error);

$res = $mysqli->query("SELECT * FROM leituras_v2 ORDER BY id DESC LIMIT 30");
$rows = [];
while($r=$res->fetch_assoc()) $rows[]=$r;
$res->free();
$mysqli->close();
$rows = array_reverse($rows);
?>
<!doctype html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="15">
  <title>Aguada - Leituras</title>
  <script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="bg-slate-50 text-slate-800">
  <div class="min-h-screen flex flex-col">
    <header class="bg-white shadow-sm border-b border-slate-200">
      <div class="max-w-6xl mx-auto px-4 py-4 flex items-center justify-between">
        <div class="flex items-center gap-3">
          <div class="h-10 w-10 rounded-lg bg-blue-600 text-white flex items-center justify-center font-semibold">AQ</div>
          <div>
            <div class="text-lg font-semibold">Aguada Telemetria</div>
            <div class="text-sm text-slate-500">Raw SensorPacketV1 (Nano/Gateway)</div>
          </div>
        </div>
      </div>
    </header>

    <main class="flex-1 max-w-6xl mx-auto w-full px-4 py-6">
      <div class="bg-white shadow-sm rounded-xl border border-slate-200 overflow-hidden">
        <div class="px-4 py-3 border-b border-slate-200 flex items-center justify-between">
          <div>
            <div class="text-base font-semibold text-slate-800">Últimas leituras</div>
            <div class="text-xs text-slate-500">Campos: version, node_id, mac, seq, distance_cm, level_cm, percentual, volume_l, vin_mv, rssi, ts_ms</div>
          </div>
          <div class="text-xs text-slate-500">Mostrando <?php echo count($rows); ?> de 30 recentes</div>
        </div>
        <div class="overflow-x-auto">
          <table class="min-w-full text-sm">
            <thead class="bg-slate-100 text-slate-700 uppercase text-xs">
              <tr>
                <th class="px-3 py-2 text-left">ts</th>
                <th class="px-3 py-2 text-left">node</th>
                <th class="px-3 py-2 text-left">mac</th>
                <th class="px-3 py-2 text-left">seq</th>
                <th class="px-3 py-2 text-left">dist (cm)</th>
                <th class="px-3 py-2 text-left">level (cm)</th>
                <th class="px-3 py-2 text-left">cap (%)</th>
                <th class="px-3 py-2 text-left">vol (L)</th>
                <th class="px-3 py-2 text-left">vin (mV)</th>
                <th class="px-3 py-2 text-left">rssi</th>
                <th class="px-3 py-2 text-left">ts_ms</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-slate-200">
            <?php foreach($rows as $r): ?>
              <tr class="hover:bg-slate-50">
                <td class="px-3 py-2 font-mono text-xs text-slate-600"><?php echo htmlspecialchars($r['ts']); ?></td>
                <td class="px-3 py-2 font-semibold"><?php echo htmlspecialchars($r['node_id']); ?></td>
                <td class="px-3 py-2 font-mono text-xs"><?php echo htmlspecialchars($r['mac']); ?></td>
                <td class="px-3 py-2 font-mono text-xs"><?php echo htmlspecialchars($r['seq']); ?></td>
                <td class="px-3 py-2"><?php echo htmlspecialchars($r['distance_cm']); ?></td>
                <td class="px-3 py-2"><?php echo htmlspecialchars($r['level_cm']); ?></td>
                <td class="px-3 py-2"><?php echo number_format((float)$r['percentual'],1); ?></td>
                <td class="px-3 py-2"><?php echo htmlspecialchars($r['volume_l']); ?></td>
                <td class="px-3 py-2"><?php echo htmlspecialchars($r['vin_mv']); ?></td>
                <td class="px-3 py-2"><?php echo htmlspecialchars($r['rssi']); ?></td>
                <td class="px-3 py-2 font-mono text-xs"><?php echo htmlspecialchars($r['ts_ms']); ?></td>
              </tr>
            <?php endforeach; ?>
            </tbody>
          </table>
        </div>
      </div>
    </main>
  </div>
</body>
</html>
