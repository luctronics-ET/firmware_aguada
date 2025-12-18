# Aguada Dashboard - Frontend com TailAdmin

Dashboard moderno para monitoramento em tempo real do sistema de telemetria Aguada.

## Tecnologias

- **TailAdmin**: Dashboard template baseado em Tailwind CSS
- **Alpine.js**: Framework reativo para JavaScript
- **ApexCharts**: Biblioteca de gráficos
- **Webpack**: Bundler de módulos

## Estrutura de Arquivos

```
frontend/aguada-dashboard/
├── src/
│   ├── aguada-telemetry.html    # Dashboard principal de telemetria
│   ├── index.html                # Dashboard original TailAdmin
│   ├── partials/                 # Componentes reutilizáveis
│   ├── js/                       # Scripts JavaScript
│   └── css/                      # Estilos personalizados
├── package.json
├── webpack.config.js
└── README.md
```

## Setup

### 1. Instalar Dependências

```bash
cd frontend/aguada-dashboard
npm install
```

### 2. Configurar Backend

Editar `aguada-telemetry.html` para apontar para o backend:

```javascript
apiBaseUrl: 'http://192.168.0.117:8080/api'
```

### 3. Desenvolvimento

```bash
npm start
```

Abre automaticamente em `http://localhost:8080`

### 4. Build para Produção

```bash
npm run build
```

Gera arquivos otimizados em `dist/`

## Integração com Backend

### Endpoints da API

O dashboard consome os seguintes endpoints:

#### 1. **GET /api/get_sensors_data.php**

Retorna status atual de todos os sensores (última leitura de cada node).

**Response:**
```json
{
  "status": "success",
  "sensors": [
    {
      "id": "NODE1",
      "node_id": 1,
      "name": "RCON",
      "mac": "20:6E:F1:6B:77:58",
      "distance_cm": 72,
      "level_cm": 398,
      "percentual": 88,
      "volume_l": 70755,
      "capacity": 80000,
      "vin_mv": 3300,
      "rssi": -45,
      "last_update": "2025-12-18 08:30:45",
      "status": "normal",
      "flags": 0,
      "alert_type": 0
    }
  ]
}
```

#### 2. **GET /api/get_recent_readings.php**

Retorna leituras recentes (últimas N leituras de todos os nodes).

**Parameters:**
- `limit` (opcional): Número de leituras (default: 50)
- `node_id` (opcional): Filtrar por node específico

**Response:**
```json
{
  "status": "success",
  "count": 20,
  "readings": [
    {
      "id": 12345,
      "created_at": "2025-12-18 08:30:45",
      "node_id": 1,
      "mac": "20:6E:F1:6B:77:58",
      "seq": 1234,
      "distance_cm": 72,
      "level_cm": 398,
      "percentual": 88,
      "volume_l": 70755,
      "vin_mv": 3300,
      "rssi": -45,
      "ts_ms": 1734510645000
    }
  ]
}
```

#### 3. **GET /api/get_history.php**

Retorna histórico agregado por minuto (médias).

**Parameters:**
- `hours` (opcional): Horas de histórico (default: 24)
- `node_id` (opcional): Filtrar por node específico

**Response:**
```json
{
  "status": "success",
  "hours": 24,
  "count": 1440,
  "history": [
    {
      "timestamp": "2025-12-18 08:30:00",
      "node_id": 1,
      "avg_level_cm": 398.5,
      "avg_percentual": 88.2,
      "avg_volume_l": 70800.0,
      "avg_rssi": -45.3,
      "reading_count": 2
    }
  ]
}
```

## Compatibilidade com Firmware

### SensorPacketV1

O dashboard é 100% compatível com o protocolo `SensorPacketV1` usado pelos nodes ESP32-C3:

```c
typedef struct __attribute__((packed)) {
    uint8_t  version;        // = 1
    uint8_t  node_id;        // 1-10
    uint8_t  mac[6];         // STA MAC
    uint32_t seq;            // Sequência
    int16_t  distance_cm;    // Distância medida
    int16_t  level_cm;       // Nível calculado
    uint8_t  percentual;     // 0-100
    uint32_t volume_l;       // Litros
    int16_t  vin_mv;         // Tensão em mV
    uint8_t  flags;          // Bit 0: is_alert
    uint8_t  alert_type;     // 0=none, 1=rapid_drop, 2=rapid_rise, 3=sensor_stuck
    int8_t   rssi;           // RSSI (gateway)
    uint32_t ts_ms;          // Timestamp (gateway)
} SensorPacketV1;
```

### Nodes Configurados

| Node ID | Nome | MAC | Reservatório | Capacidade | Tipo |
|---------|------|-----|--------------|------------|------|
| 1 | RCON | 20:6E:F1:6B:77:58 | Consumo | 80,000L | ESP32-C3 |
| 2 | RCAV | DC:06:75:67:6A:CC | Incêndio (Avenida) | 80,000L | ESP32-C3 |
| 3 | RCB3 | 80:F1:B2:50:31:34 | Casa de Bombas 03 | 80,000L | ESP32-C3 |
| 4 | CIE1 | DC:B4:D9:8B:9E:AC | Castelo Incêndio Elevado 1 | 245,000L | ESP32-C3 |
| 5 | CIE2 | DC:B4:D9:8B:9E:AC | Castelo Incêndio Elevado 2 | 245,000L | ESP32-C3 |
| 10 | RCON-ETH | AA:BB:CC:DD:EE:01 | Consumo Backup | 80,000L | Nano ETH |

## Features

### Dashboard Principal (aguada-telemetry.html)

✅ **Cards de Status por Node**
- Nome do reservatório
- Status online/offline (3 min threshold)
- Percentual de nível com progress bar colorido
- Volume atual e capacidade
- Distância do sensor
- RSSI (qualidade de sinal)
- Última atualização
- Badges de alerta (se houver)

✅ **Gráficos em Tempo Real**
- Histórico de níveis (24h)
- Distribuição de volume por reservatório
- Qualidade de sinal (RSSI) ao longo do tempo

✅ **Tabela de Leituras Recentes**
- Últimas 20 leituras de todos os nodes
- Filtros e ordenação
- Export para CSV (planejado)

✅ **Auto-Refresh**
- Atualização automática a cada 5 segundos
- Indicador visual de conexão
- Cache local para performance

### Alertas e Notificações

- **Verde (≥75%)**: Nível normal
- **Azul (50-74%)**: Nível médio
- **Amarelo (25-49%)**: Atenção - nível baixo
- **Vermelho (<25%)**: Alerta crítico

### Detecção de Anomalias

O sistema detecta automaticamente:
- **Sensor Stuck**: Leitura inalterada por 5+ ciclos (alert_type=3)
- **Rapid Drop**: Nível caiu >10cm (alert_type=1)
- **Rapid Rise**: Nível subiu >10cm (alert_type=2)

## Customização

### Mudar Cores do Tema

Editar `src/css/style.css`:

```css
:root {
  --primary-color: #667eea;
  --success-color: #28a745;
  --warning-color: #ffc107;
  --danger-color: #dc3545;
}
```

### Adicionar Novo Gráfico

1. Criar div container em `aguada-telemetry.html`
2. Implementar função em `updateCharts()`
3. Usar ApexCharts para renderização

Exemplo:
```javascript
updateCharts() {
  const chart = new ApexCharts(document.querySelector("#newChart"), {
    series: [{
      name: 'Série',
      data: this.nodes.map(n => n.level_cm)
    }],
    chart: {
      type: 'line',
      height: 350
    }
  });
  chart.render();
}
```

### Adicionar Notificações Push

TODO: Implementar via WebSocket ou Server-Sent Events (SSE)

## Deploy

### Opção 1: Servir via Backend PHP

Copiar `dist/` para `/var/www/html/aguada`:

```bash
npm run build
sudo cp -r dist/* /var/www/html/aguada/
```

Acessar: `http://192.168.0.117/aguada/aguada-telemetry.html`

### Opção 2: Nginx

```nginx
server {
    listen 80;
    server_name aguada.local;
    root /home/luciano/firmware_aguada/frontend/aguada-dashboard/dist;
    index aguada-telemetry.html;
    
    location /api {
        proxy_pass http://192.168.0.117:8080;
    }
}
```

### Opção 3: Docker

TODO: Criar Dockerfile

## Troubleshooting

### CORS Errors

Certificar que backend tem headers corretos em todas as APIs:

```php
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');
```

### Dados Não Carregam

1. Verificar se backend está rodando: `curl http://192.168.0.117:8080/api/get_sensors_data.php`
2. Verificar MySQL: `systemctl status mysql`
3. Verificar logs do navegador (F12 > Console)
4. Verificar gateway: `cd gateway_devkit_v1 && idf.py monitor`

### Gráficos Não Aparecem

1. Verificar se ApexCharts está carregado
2. Abrir console e verificar erros
3. Testar com dados mock

## Próximos Passos

- [ ] Implementar gráficos ApexCharts completos
- [ ] Adicionar WebSocket para updates em tempo real
- [ ] Criar página de configuração de alertas
- [ ] Implementar exportação de dados (CSV, PDF)
- [ ] Adicionar autenticação (login/logout)
- [ ] Criar dashboard SCADA completo
- [ ] Integrar com sistema de balanço hídrico
- [ ] Mobile app (PWA)

## Referências

- [TailAdmin Docs](https://tailadmin.com/docs)
- [Alpine.js](https://alpinejs.dev/)
- [ApexCharts](https://apexcharts.com/)
- [Tailwind CSS](https://tailwindcss.com/)
