#!/bin/bash
# Script simplificado de inicialização (usa backend/database existentes)

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Sistema Aguada - Start Rápido"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 1. Verificar MySQL
echo "[1/4] Verificando MySQL..."
if systemctl is-active --quiet mysql; then
    echo "✅ MySQL rodando"
else
    echo "❌ MySQL não está rodando. Execute: sudo systemctl start mysql"
    exit 1
fi

# 2. Iniciar backend PHP
echo ""
echo "[2/4] Iniciando backend PHP..."
if [ -f .php_server.pid ]; then
    PID=$(cat .php_server.pid)
    if ps -p $PID > /dev/null 2>&1; then
        echo "✅ Backend PHP já rodando (PID: $PID)"
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

# 3. Verificar frontend dependencies
echo ""
echo "[3/4] Verificando frontend..."
if [ ! -d "frontend/aguada-dashboard/node_modules" ]; then
    echo "⚙️  Instalando dependências (npm install)..."
    cd frontend/aguada-dashboard
    npm install --silent
    cd ../..
    echo "✅ Dependências instaladas"
else
    echo "✅ Dependências instaladas"
fi

# 4. Iniciar frontend
echo ""
echo "[4/4] Iniciando frontend..."
cd frontend/aguada-dashboard

# Matar processo anterior se existir
pkill -f "webpack serve" 2>/dev/null || true

# Iniciar em background
npm start > ../../frontend.log 2>&1 &
FRONTEND_PID=$!
cd ../..

sleep 3
echo "✅ Frontend iniciado (PID: $FRONTEND_PID)"

# Resumo
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ✅ SISTEMA INICIADO!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📊 URLs:"
echo "   Backend:  http://192.168.0.117:8080"
echo "   Frontend: http://localhost:8080/aguada-telemetry.html"
echo ""
echo "📋 Logs:"
echo "   Backend:  tail -f server.log"
echo "   Frontend: tail -f frontend.log"
echo ""
echo "⏹️  Parar: ./stop_services.sh"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
