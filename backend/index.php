<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aguada Telemetry - Status</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 800px;
            width: 100%;
        }
        h1 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 2.5em;
        }
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 1.1em;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin: 30px 0;
        }
        .status-card {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 20px;
            text-align: center;
            transition: transform 0.2s;
        }
        .status-card:hover {
            transform: translateY(-5px);
        }
        .status-icon {
            font-size: 3em;
            margin-bottom: 10px;
        }
        .status-title {
            font-weight: 600;
            color: #333;
            margin-bottom: 5px;
        }
        .status-value {
            color: #666;
            font-size: 0.9em;
        }
        .success { color: #28a745; }
        .warning { color: #ffc107; }
        .info { color: #17a2b8; }
        .section {
            margin: 30px 0;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 12px;
        }
        .section h2 {
            color: #333;
            margin-bottom: 15px;
            font-size: 1.5em;
        }
        .command {
            background: #2d3748;
            color: #68d391;
            padding: 15px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            margin: 10px 0;
            overflow-x: auto;
        }
        .button {
            background: #667eea;
            color: white;
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            cursor: pointer;
            transition: background 0.2s;
            text-decoration: none;
            display: inline-block;
            margin: 5px;
        }
        .button:hover {
            background: #5568d3;
        }
        .button.secondary {
            background: #6c757d;
        }
        .button.secondary:hover {
            background: #545b62;
        }
        ul {
            list-style-position: inside;
            color: #555;
            line-height: 1.8;
        }
        .footer {
            margin-top: 30px;
            text-align: center;
            color: #999;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌊 Aguada Telemetry</h1>
        <p class="subtitle">Sistema de Monitoramento de Nível de Água</p>

        <div class="status-grid">
            <div class="status-card">
                <div class="status-icon success">✅</div>
                <div class="status-title">Backend PHP</div>
                <div class="status-value">Rodando na porta 8080</div>
            </div>
            <div class="status-card">
                <div class="status-icon warning">⚠️</div>
                <div class="status-title">MySQL</div>
                <div class="status-value">Aguardando configuração</div>
            </div>
            <div class="status-card">
                <div class="status-icon info">📡</div>
                <div class="status-title">Nós Sensores</div>
                <div class="status-value">5 dispositivos ESP32</div>
            </div>
            <div class="status-card">
                <div class="status-icon info">🔧</div>
                <div class="status-title">Gateway</div>
                <div class="status-value">ESP32 DevKit V1</div>
            </div>
        </div>

        <div class="section">
            <h2>🚀 Próximos Passos</h2>
            <ul>
                <li><strong>Iniciar MySQL:</strong> Banco de dados necessário para armazenar telemetria</li>
                <li><strong>Importar Schema:</strong> Criar tabela leituras_v2</li>
                <li><strong>Configurar Gateway:</strong> Apontar para http://localhost:8080/ingest_sensorpacket.php</li>
                <li><strong>Testar Pipeline:</strong> Verificar dados chegando dos sensores</li>
            </ul>
        </div>

        <div class="section">
            <h2>💾 Setup do Banco de Dados</h2>
            <p style="margin-bottom: 15px;"><strong>1. Iniciar MySQL/MariaDB:</strong></p>
            <div class="command">sudo systemctl start mysql</div>
            
            <p style="margin: 15px 0;"><strong>2. Criar banco e importar schema:</strong></p>
            <div class="command">mysql -u root -p -e "CREATE DATABASE sensores_db;"<br>mysql -u root -p sensores_db < database/schema.sql</div>
            
            <p style="margin: 15px 0;"><strong>3. Verificar instalação:</strong></p>
            <div class="command">mysql -u root -p sensores_db -e "SHOW TABLES;"</div>
        </div>

        <div class="section">
            <h2>📊 Links Úteis</h2>
            <a href="dashboard.php" class="button">Dashboard (requer DB)</a>
            <a href="ingest_sensorpacket.php" class="button secondary">Endpoint de Ingestão</a>
            <a href="../database/README.md" class="button secondary">Docs Database</a>
        </div>

        <div class="section">
            <h2>📁 Estrutura do Projeto</h2>
            <div class="command" style="color: #a0aec0; line-height: 1.6;">
firmware_aguada/
├── backend/         ← Você está aqui
│   ├── config.php
│   ├── ingest_sensorpacket.php
│   └── dashboard.php
├── database/
│   └── schema.sql   ← Importar este arquivo
├── frontend/        ← Dashboard web (futuro)
├── node_ultra1/     ← Firmware dos sensores
└── gateway_devkit_v1/  ← Firmware do gateway
            </div>
        </div>

        <div class="footer">
            <p>Aguada Telemetry System v1.0 | ESP32 + ESP-NOW + PHP/MySQL</p>
            <p>Servidor rodando em: <strong>http://localhost:8080</strong></p>
        </div>
    </div>
</body>
</html>
