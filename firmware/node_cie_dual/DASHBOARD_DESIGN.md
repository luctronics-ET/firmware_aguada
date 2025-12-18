# Dashboard UI - Cisterna Ilha do Engenho
## Design Compacto para Monitoramento em Tempo Real

### 📊 Layout Proposto

```
╔═══════════════════════════════════════════════════════════════════════╗
║  Cisterna Ilha do Engenho - Monitoramento Tempo Real                 ║
║  Atualizado: 17/12/2025 14:23:45                                     ║
╠═══════════════════════════════════════════════════════════════════════╣
║                                                                       ║
║  ┌────────────────────┐          ┌────────────────────┐              ║
║  │ CIE1 ▓▓▓▓▓▓▓░░░ 72%│          │ CIE2 ▓▓▓▓▓▓░░░░ 68%│              ║
║  │ 176/245 m³         │          │ 167/245 m³         │              ║
║  │ ↑ 20m³ ↓ 100m³    │          │ ↑ 15m³ ↓ 95m³     │              ║
║  │ 🟢 14:23 | ⚡ -45dBm│          │ 🟢 14:23 | ⚡ -48dBm│              ║
║  └────────────────────┘          └────────────────────┘              ║
║                                                                       ║
║  Total: 343/490 m³ (70%) | Dia: ↑35m³ ↓195m³ | Saldo: -160m³       ║
╚═══════════════════════════════════════════════════════════════════════╝
```

### 🎨 Versão com Status Visual (Cores)

```
╔═══════════════════════════════════════════════════════════════════════╗
║  🌊 Cisterna Ilha do Engenho                      🔄 Atualizado: 14:23 ║
╠═══════════════════════════════════════════════════════════════════════╣
║                                                                       ║
║  ┌──── CIE1 ──────────┐    ┌──── CIE2 ──────────┐                   ║
║  │ ▓▓▓▓▓▓▓▓░░░░░ 72%  │    │ ▓▓▓▓▓▓░░░░░░░ 68%  │  [verde: >50%]   ║
║  │ 176m³ / 245m³      │    │ 167m³ / 245m³      │  [amarelo: 20-50%]║
║  │                    │    │                    │  [vermelho: <20%] ║
║  │ ↗️ 20m³  ↘️ 100m³    │    │ ↗️ 15m³  ↘️ 95m³     │                   ║
║  │ 🟢 14:23  -45dBm   │    │ 🟢 14:22  -48dBm   │                   ║
║  └────────────────────┘    └────────────────────┘                   ║
║                                                                       ║
║  📊 Resumo: 343m³/490m³ (70%)  |  💧 Hoje: +35m³ -195m³ = -160m³    ║
╚═══════════════════════════════════════════════════════════════════════╝
```

### 🚨 Card com Alerta (Exemplo)

```
╔═══════════════════════════════════════════════════════════════════════╗
║  ┌──── CIE1 ──────────┐    ┌──── CIE2 ──────────┐                   ║
║  │ 🚨 ALERTA!         │    │ ▓▓▓▓▓▓░░░░░░░ 68%  │                   ║
║  │ ▓▓░░░░░░░░░░░ 15%  │    │ 167m³ / 245m³      │                   ║
║  │ 37m³ / 245m³       │    │                    │                   ║
║  │                    │    │ ↗️ 15m³  ↘️ 95m³     │                   ║
║  │ ↗️ 5m³  ↘️ 120m³     │    │ 🟢 14:22  -48dBm   │                   ║
║  │ 🔴 14:20  -42dBm   │    │                    │                   ║
║  │ ⚠️ Vazamento!      │    └────────────────────┘                   ║
║  └────────────────────┘                                              ║
╚═══════════════════════════════════════════════════════════════════════╝
```

### 💤 Card Offline (Sensor não responde)

```
╔═══════════════════════════════════════════════════════════════════════╗
║  ┌──── CIE1 ──────────┐    ┌──── CIE2 ──────────┐                   ║
║  │ ▒▒▒▒▒▒▒▒░░░░░ 72%  │    │ ▓▓▓▓▓▓░░░░░░░ 68%  │  [▒ = offline]   ║
║  │ 176m³ / 245m³      │    │ 167m³ / 245m³      │  [▓ = online]    ║
║  │                    │    │                    │                   ║
║  │ ↗️ --  ↘️ --        │    │ ↗️ 15m³  ↘️ 95m³     │                   ║
║  │ ⚫ 12:05  -45dBm   │    │ 🟢 14:22  -48dBm   │                   ║
║  │ 🔌 Offline 2h18min │    │                    │                   ║
║  └────────────────────┘    └────────────────────┘                   ║
╚═══════════════════════════════════════════════════════════════════════╝
```

---

## 📐 Especificações Técnicas do Card

### Estrutura do Card (Compacto)

```
┌──── [NOME] ──────────┐
│ [BARGRAPH] [PCT%]    │  Linha 1: Barra + Percentual
│ [VOL]/[MAX] m³       │  Linha 2: Volume atual/máximo
│                      │  Linha 3: (espaço para respirar)
│ ↗️ [IN]m³ ↘️ [OUT]m³   │  Linha 4: Entradas/Saídas
│ [ST] [TIME] [RSSI]   │  Linha 5: Status + Hora + Sinal
│ [ALERTA] (opcional)  │  Linha 6: Mensagem de alerta
└──────────────────────┘
```

### Dimensões
- **Largura**: 22 caracteres
- **Altura**: 6-7 linhas (dependendo se tem alerta)
- **Espaçamento**: 4 espaços entre cards

### Ícones e Símbolos

| Elemento | Símbolo | Cor CSS | Significado |
|----------|---------|---------|-------------|
| **Abastecimento** | ↗️ ou ⬆️ | `#28a745` (verde) | Volume de entrada |
| **Consumo** | ↘️ ou ⬇️ | `#dc3545` (vermelho) | Volume de saída |
| **Online** | 🟢 | `#28a745` | Última leitura < 5min |
| **Alerta** | 🟡 | `#ffc107` | Última leitura 5-15min |
| **Offline** | 🔴 ou ⚫ | `#6c757d` | Sem dados > 15min |
| **Sinal WiFi** | ⚡ | Gradiente | RSSI em dBm |
| **Alerta Crítico** | 🚨 | `#dc3545` pulsante | Vazamento/Inundação |
| **Manutenção** | 🔧 | `#17a2b8` | Sensor em manutenção |

### Bargraph (Barra de Progresso)

```
Preenchimento:
▓▓▓▓▓▓▓▓░░░░░  (▓ = cheio, ░ = vazio)

Cores por nível:
> 70%:  Verde   #28a745
50-70%: Azul    #17a2b8
30-50%: Amarelo #ffc107
< 30%:  Laranja #fd7e14
< 15%:  Vermelho #dc3545

Offline: Cinza translúcido #6c757d80 (50% opacidade)
```

### Estados do Card

#### 1. **Normal (Online)**
```css
background: white
border: 2px solid #28a745
box-shadow: 0 2px 4px rgba(0,0,0,0.1)
```

#### 2. **Alerta (Anomalia)**
```css
background: #fff3cd
border: 2px solid #ffc107
box-shadow: 0 0 10px rgba(255,193,7,0.5)
animation: pulse 2s infinite
```

#### 3. **Crítico (Vazamento)**
```css
background: #f8d7da
border: 3px solid #dc3545
box-shadow: 0 0 15px rgba(220,53,69,0.7)
animation: pulse-fast 1s infinite
```

#### 4. **Offline (Sem dados)**
```css
background: rgba(108,117,125,0.1)
border: 2px dashed #6c757d
opacity: 0.7
filter: grayscale(50%)
```

---

## 💻 HTML/CSS Exemplo

### Estrutura HTML

```html
<div class="cistern-container">
    <div class="cistern-header">
        <h2>🌊 Cisterna Ilha do Engenho</h2>
        <span class="last-update">🔄 14:23:45</span>
    </div>
    
    <div class="reservoirs">
        <!-- CIE1 -->
        <div class="reservoir-card online" id="cie1">
            <div class="card-header">CIE1</div>
            
            <div class="progress-bar">
                <div class="progress-fill" style="width: 72%; background: #28a745;">
                    <span class="progress-text">72%</span>
                </div>
            </div>
            
            <div class="volume">176m³ / 245m³</div>
            
            <div class="flow">
                <span class="inflow">↗️ 20m³</span>
                <span class="outflow">↘️ 100m³</span>
            </div>
            
            <div class="status-bar">
                <span class="status online">🟢</span>
                <span class="time">14:23</span>
                <span class="signal">⚡ -45dBm</span>
            </div>
        </div>
        
        <!-- CIE2 -->
        <div class="reservoir-card online" id="cie2">
            <div class="card-header">CIE2</div>
            
            <div class="progress-bar">
                <div class="progress-fill" style="width: 68%; background: #28a745;">
                    <span class="progress-text">68%</span>
                </div>
            </div>
            
            <div class="volume">167m³ / 245m³</div>
            
            <div class="flow">
                <span class="inflow">↗️ 15m³</span>
                <span class="outflow">↘️ 95m³</span>
            </div>
            
            <div class="status-bar">
                <span class="status online">🟢</span>
                <span class="time">14:22</span>
                <span class="signal">⚡ -48dBm</span>
            </div>
        </div>
    </div>
    
    <div class="summary">
        📊 Total: 343m³/490m³ (70%) | 💧 Hoje: ↗️35m³ ↘️195m³ = -160m³
    </div>
</div>
```

### CSS Compacto

```css
.cistern-container {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    max-width: 600px;
    margin: 20px auto;
    padding: 15px;
    background: #f8f9fa;
    border-radius: 8px;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
}

.cistern-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 15px;
    padding-bottom: 10px;
    border-bottom: 2px solid #dee2e6;
}

.cistern-header h2 {
    margin: 0;
    font-size: 18px;
}

.last-update {
    font-size: 12px;
    color: #6c757d;
}

.reservoirs {
    display: flex;
    gap: 20px;
    margin-bottom: 15px;
}

.reservoir-card {
    flex: 1;
    padding: 12px;
    background: white;
    border-radius: 6px;
    border: 2px solid #28a745;
    min-height: 140px;
    transition: all 0.3s ease;
}

.reservoir-card.offline {
    background: rgba(108,117,125,0.1);
    border: 2px dashed #6c757d;
    opacity: 0.7;
}

.reservoir-card.alert {
    background: #fff3cd;
    border: 2px solid #ffc107;
    animation: pulse 2s infinite;
}

.reservoir-card.critical {
    background: #f8d7da;
    border: 3px solid #dc3545;
    animation: pulse-fast 1s infinite;
}

@keyframes pulse {
    0%, 100% { box-shadow: 0 0 10px rgba(255,193,7,0.3); }
    50% { box-shadow: 0 0 20px rgba(255,193,7,0.6); }
}

@keyframes pulse-fast {
    0%, 100% { box-shadow: 0 0 15px rgba(220,53,69,0.5); }
    50% { box-shadow: 0 0 25px rgba(220,53,69,0.9); }
}

.card-header {
    font-weight: bold;
    font-size: 14px;
    margin-bottom: 8px;
    text-align: center;
    border-bottom: 1px solid #dee2e6;
    padding-bottom: 4px;
}

.progress-bar {
    width: 100%;
    height: 20px;
    background: #e9ecef;
    border-radius: 10px;
    overflow: hidden;
    margin-bottom: 8px;
    position: relative;
}

.progress-fill {
    height: 100%;
    transition: width 0.5s ease;
    display: flex;
    align-items: center;
    justify-content: center;
}

.progress-text {
    font-size: 11px;
    font-weight: bold;
    color: white;
    text-shadow: 0 1px 2px rgba(0,0,0,0.3);
}

.volume {
    font-size: 13px;
    font-weight: bold;
    text-align: center;
    margin-bottom: 8px;
}

.flow {
    display: flex;
    justify-content: space-around;
    font-size: 12px;
    margin-bottom: 8px;
}

.inflow {
    color: #28a745;
}

.outflow {
    color: #dc3545;
}

.status-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 11px;
    padding-top: 6px;
    border-top: 1px solid #dee2e6;
}

.status {
    font-size: 14px;
}

.time {
    color: #6c757d;
}

.signal {
    color: #17a2b8;
}

.summary {
    text-align: center;
    padding: 10px;
    background: white;
    border-radius: 6px;
    font-size: 13px;
    border: 1px solid #dee2e6;
}
```

---

## 📱 Versão Mobile Responsiva

```css
@media (max-width: 600px) {
    .reservoirs {
        flex-direction: column;
        gap: 15px;
    }
    
    .reservoir-card {
        min-height: auto;
    }
    
    .cistern-header h2 {
        font-size: 16px;
    }
    
    .summary {
        font-size: 11px;
    }
}
```

---

## 🔧 Backend PHP - Cálculo de In/Out

```php
<?php
// Calcular abastecimento e consumo do dia (00:00 - 23:59)

function getDailyFlow($node_id, $date = null) {
    global $pdo;
    
    if ($date === null) {
        $date = date('Y-m-d');
    }
    
    $start_ts = strtotime($date . ' 00:00:00') * 1000;
    $end_ts = strtotime($date . ' 23:59:59') * 1000;
    
    // Obter todas as leituras do dia ordenadas
    $stmt = $pdo->prepare("
        SELECT volume_l, ts_ms
        FROM telemetry_processed
        WHERE node_id = ?
          AND ts_ms >= ?
          AND ts_ms <= ?
        ORDER BY ts_ms ASC
    ");
    $stmt->execute([$node_id, $start_ts, $end_ts]);
    $readings = $stmt->fetchAll(PDO::FETCH_ASSOC);
    
    if (count($readings) < 2) {
        return ['inflow' => 0, 'outflow' => 0, 'net' => 0];
    }
    
    $inflow = 0;  // Abastecimento (volume aumentou)
    $outflow = 0; // Consumo (volume diminuiu)
    
    for ($i = 1; $i < count($readings); $i++) {
        $delta = $readings[$i]['volume_l'] - $readings[$i-1]['volume_l'];
        
        if ($delta > 0) {
            $inflow += $delta;
        } else if ($delta < 0) {
            $outflow += abs($delta);
        }
    }
    
    return [
        'inflow' => round($inflow / 1000, 1),   // Converter para m³
        'outflow' => round($outflow / 1000, 1), // Converter para m³
        'net' => round(($inflow - $outflow) / 1000, 1)
    ];
}

// Determinar status do sensor (online/offline)
function getSensorStatus($node_id) {
    global $pdo;
    
    $stmt = $pdo->prepare("
        SELECT ts_ms, flags, alert_type, rssi
        FROM telemetry_processed
        WHERE node_id = ?
        ORDER BY ts_ms DESC
        LIMIT 1
    ");
    $stmt->execute([$node_id]);
    $last = $stmt->fetch(PDO::FETCH_ASSOC);
    
    if (!$last) {
        return ['status' => 'offline', 'offline_minutes' => 9999];
    }
    
    $now = time() * 1000;
    $elapsed_ms = $now - $last['ts_ms'];
    $elapsed_min = floor($elapsed_ms / 60000);
    
    if ($elapsed_min < 5) {
        $status = 'online';
    } else if ($elapsed_min < 15) {
        $status = 'warning';
    } else {
        $status = 'offline';
    }
    
    return [
        'status' => $status,
        'offline_minutes' => $elapsed_min,
        'last_ts' => $last['ts_ms'],
        'has_alert' => ($last['flags'] & 1) ? true : false,
        'alert_type' => $last['alert_type'],
        'rssi' => $last['rssi']
    ];
}

// API endpoint: /api/dashboard.php
header('Content-Type: application/json');

$node_ids = [4, 5]; // CIE1 e CIE2
$data = [];

foreach ($node_ids as $node_id) {
    $flow = getDailyFlow($node_id);
    $status = getSensorStatus($node_id);
    
    // Última leitura
    $stmt = $pdo->prepare("
        SELECT level_cm, percentual, volume_l, distance_cm
        FROM telemetry_processed
        WHERE node_id = ?
        ORDER BY ts_ms DESC
        LIMIT 1
    ");
    $stmt->execute([$node_id]);
    $current = $stmt->fetch(PDO::FETCH_ASSOC);
    
    $data["cie" . $node_id] = [
        'node_id' => $node_id,
        'volume_m3' => round($current['volume_l'] / 1000, 0),
        'max_m3' => 245,
        'percentual' => $current['percentual'],
        'level_cm' => $current['level_cm'],
        'inflow_m3' => $flow['inflow'],
        'outflow_m3' => $flow['outflow'],
        'net_m3' => $flow['net'],
        'status' => $status['status'],
        'offline_minutes' => $status['offline_minutes'],
        'last_time' => date('H:i', $status['last_ts'] / 1000),
        'has_alert' => $status['has_alert'],
        'alert_type' => $status['alert_type'],
        'rssi' => $status['rssi']
    ];
}

echo json_encode($data, JSON_PRETTY_PRINT);
?>
```

---

## 🎯 JavaScript - Atualização Dinâmica

```javascript
// dashboard.js

async function updateDashboard() {
    try {
        const response = await fetch('/api/dashboard.php');
        const data = await response.json();
        
        updateReservoir('cie1', data.cie4);
        updateReservoir('cie2', data.cie5);
        updateSummary(data);
        
    } catch (error) {
        console.error('Erro ao atualizar dashboard:', error);
    }
}

function updateReservoir(elementId, data) {
    const card = document.getElementById(elementId);
    
    // Atualizar barra de progresso
    const progressFill = card.querySelector('.progress-fill');
    progressFill.style.width = data.percentual + '%';
    progressFill.style.background = getColorByLevel(data.percentual);
    progressFill.querySelector('.progress-text').textContent = data.percentual + '%';
    
    // Atualizar volume
    card.querySelector('.volume').textContent = 
        `${data.volume_m3}m³ / ${data.max_m3}m³`;
    
    // Atualizar fluxo
    card.querySelector('.inflow').textContent = `↗️ ${data.inflow_m3}m³`;
    card.querySelector('.outflow').textContent = `↘️ ${data.outflow_m3}m³`;
    
    // Atualizar status
    const statusIcon = card.querySelector('.status');
    const statusTime = card.querySelector('.time');
    const statusSignal = card.querySelector('.signal');
    
    if (data.status === 'online') {
        statusIcon.textContent = '🟢';
        card.className = 'reservoir-card online';
    } else if (data.status === 'warning') {
        statusIcon.textContent = '🟡';
        card.className = 'reservoir-card alert';
    } else {
        statusIcon.textContent = '⚫';
        card.className = 'reservoir-card offline';
        statusTime.textContent = `Offline ${data.offline_minutes}min`;
    }
    
    if (data.has_alert) {
        card.className = 'reservoir-card critical';
    }
    
    statusTime.textContent = data.last_time;
    statusSignal.textContent = `⚡ ${data.rssi}dBm`;
}

function getColorByLevel(percent) {
    if (percent >= 70) return '#28a745';  // Verde
    if (percent >= 50) return '#17a2b8';  // Azul
    if (percent >= 30) return '#ffc107';  // Amarelo
    if (percent >= 15) return '#fd7e14';  // Laranja
    return '#dc3545';  // Vermelho
}

function updateSummary(data) {
    const cie1 = data.cie4;
    const cie2 = data.cie5;
    
    const totalVolume = cie1.volume_m3 + cie2.volume_m3;
    const totalMax = cie1.max_m3 + cie2.max_m3;
    const totalPercent = Math.round((totalVolume / totalMax) * 100);
    
    const totalInflow = cie1.inflow_m3 + cie2.inflow_m3;
    const totalOutflow = cie1.outflow_m3 + cie2.outflow_m3;
    const totalNet = totalInflow - totalOutflow;
    
    document.querySelector('.summary').innerHTML = 
        `📊 Total: ${totalVolume}m³/${totalMax}m³ (${totalPercent}%) | ` +
        `💧 Hoje: ↗️${totalInflow}m³ ↘️${totalOutflow}m³ = ${totalNet >= 0 ? '+' : ''}${totalNet}m³`;
}

// Atualizar a cada 30 segundos
setInterval(updateDashboard, 30000);
updateDashboard(); // Primeira atualização imediata
```

---

## 📊 Resumo das Features

### ✅ Implementado no Design

- **Cards Compactos**: Máximo de informação em espaço mínimo
- **Bargraph Visual**: Barra de progresso colorida (▓░)
- **Ícones Intuitivos**: ↗️ abastecimento (verde), ↘️ consumo (vermelho)
- **Status Online/Offline**: 🟢 🟡 ⚫ com tempo de última leitura
- **Alertas Visuais**: Animações pulsantes para anomalias
- **RSSI**: Indicador de qualidade de sinal WiFi
- **Volume Diário**: Cálculo de in/out do dia (00:00-23:59)
- **Totalizador**: Soma dos 2 reservatórios + saldo líquido
- **Responsivo**: Layout adaptável para mobile

### 🎯 Lógica de Cores

- Verde (`#28a745`): Nível > 70% ou abastecimento
- Azul (`#17a2b8`): Nível 50-70%
- Amarelo (`#ffc107`): Nível 30-50% ou alerta
- Laranja (`#fd7e14`): Nível < 30%
- Vermelho (`#dc3545`): Nível < 15%, vazamento ou consumo
- Cinza (`#6c757d`): Offline ou sem dados

### 🚀 Próximos Passos

1. Copiar HTML/CSS/JS para arquivos do backend
2. Ajustar endpoint `/api/dashboard.php`
3. Testar com dados reais do MySQL
4. Adicionar gráficos históricos (Chart.js)
5. Implementar notificações push para alertas

Este design maximiza densidade de informação sem poluição visual! 🎯✨
