#!/bin/bash
# Script de inicialização completa do Sistema Aguada
# Executa: Backend + Frontend + Configuração inicial

set -e  # Exit on error

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Sistema Aguada - Inicialização Completa"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 1. Verificar MySQL
echo "[1/6] Verificando MySQL..."
if systemctl is-active --quiet mysql; then
    echo "✅ MySQL já está rodando"
else
    echo "⚙️  Iniciando MySQL..."
    sudo systemctl start mysql
    sleep 2
    echo "✅ MySQL iniciado"
fi

# 2. Verificar database aguada_db
echo ""
echo "[2/6] Verificando database aguada_db..."
DB_EXISTS=$(sudo mysql -e "SHOW DATABASES LIKE 'aguada_db';" 2>/dev/null | grep aguada_db || true)

if [ -z "$DB_EXISTS" ]; then
    echo "⚙️  Criando database aguada_db..."
    sudo mysql -e "CREATE DATABASE aguada_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
    
    echo "⚙️  Importando schema.sql..."
    sudo mysql aguada_db < database/schema.sql
    
    echo "✅ Database criada e schema importado"
else
    echo "✅ Database aguada_db já existe"
fi

# 3. Executar configuração dos nodes (se não executado)
echo ""
echo "[3/6] Configurando nodes..."
NODE_CONFIG_EXISTS=$(sudo mysql aguada_db -e "SELECT COUNT(*) FROM node_configs WHERE node_id IN (1,2,3,4,5,10);" 2>/dev/null | tail -n 1 || echo "0")

if [ "$NODE_CONFIG_EXISTS" -lt 6 ]; then
    echo "⚙️  Importando nodes_config_REAL_MACS.sql..."
    sudo mysql aguada_db < firmware/nodes_config_REAL_MACS.sql
    echo "✅ Configuração de nodes importada"
else
    echo "✅ Nodes já configurados"
fi

# 4. Iniciar backend PHP
echo ""
echo "[4/6] Iniciando backend PHP..."
if [ -f .php_server.pid ]; then
    PID=$(cat .php_server.pid)
    if ps -p $PID > /dev/null 2>&1; then
        echo "✅ Backend PHP já está rodando (PID: $PID)"
    else
        rm .php_server.pid
        php -S 192.168.0.117:8080 -t backend > server.log 2>&1 &
        echo $! > .php_server.pid
        sleep 2
        echo "✅ Backend PHP iniciado (PID: $(cat .php_server.pid))"
    fi
else
    php -S 192.168.0.117:8080 -t backend > server.log 2>&1 &
    echo $! > .php_server.pid
    sleep 2
    echo "✅ Backend PHP iniciado (PID: $(cat .php_server.pid))"
fi

# 5. Verificar node_modules do frontend
echo ""
echo "[5/6] Verificando dependências do frontend..."
if [ ! -d "frontend/aguada-dashboard/node_modules" ]; then
    echo "⚙️  Instalando dependências (npm install)..."
    cd frontend/aguada-dashboard
    npm install --silent
    cd ../..
    echo "✅ Dependências instaladas"
else
    echo "✅ Dependências já instaladas"
fi

# 6. Iniciar frontend
echo ""
echo "[6/6] Iniciando frontend..."
echo "⚙️  Webpack dev server..."

cd frontend/aguada-dashboard
npm start &
FRONTEND_PID=$!
cd ../..

echo "✅ Frontend iniciado (PID: $FRONTEND_PID)"

# Resumo
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ✅ SISTEMA INICIALIZADO COM SUCESSO!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📊 URLs de Acesso:"
echo "   Backend:  http://192.168.0.117:8080"
echo "   Frontend: http://localhost:8080/aguada-telemetry.html"
echo ""
echo "📡 Endpoints da API:"
echo "   GET http://192.168.0.117:8080/api/get_sensors_data.php"
echo "   GET http://192.168.0.117:8080/api/get_recent_readings.php"
echo "   GET http://192.168.0.117:8080/api/get_history.php"
echo ""
echo "📋 Comandos úteis:"
echo "   ./status_services.sh       - Ver status de todos os serviços"
echo "   ./stop_services.sh         - Parar todos os serviços"
echo "   tail -f server.log         - Ver logs do backend"
echo ""
echo "🔧 Próximos passos:"
echo "   1. Conectar sensores HC-SR04 nos nodes"
echo "   2. Verificar gateway recebendo dados"
echo "   3. Validar dados no dashboard"
echo ""
echo "Pressione Ctrl+C para parar todos os serviços"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Aguardar Ctrl+C
wait $FRONTEND_PID
