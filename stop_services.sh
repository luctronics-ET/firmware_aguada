#!/bin/bash
# Script para parar todos os serviços do Aguada Telemetry

echo "🛑 Parando Aguada Telemetry Services..."
echo ""

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$PROJECT_DIR"

# Cores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 1. Parar servidor PHP
if [ -f ".php_server.pid" ]; then
    PHP_PID=$(cat .php_server.pid)
    if ps -p $PHP_PID > /dev/null 2>&1; then
        echo -e "${YELLOW}⚙️  Parando servidor PHP (PID: $PHP_PID)...${NC}"
        kill $PHP_PID 2>/dev/null
        rm .php_server.pid
        echo -e "${GREEN}✅ Servidor PHP parado${NC}"
    else
        echo "ℹ️  Servidor PHP não está rodando"
        rm .php_server.pid
    fi
else
    # Fallback: tentar matar todos os processos PHP na porta 8080
    if pgrep -f "php -S.*8080" > /dev/null; then
        echo -e "${YELLOW}⚙️  Parando servidores PHP na porta 8080...${NC}"
        pkill -f "php -S.*8080"
        echo -e "${GREEN}✅ Servidores PHP parados${NC}"
    else
        echo "ℹ️  Nenhum servidor PHP rodando na porta 8080"
    fi
fi
echo ""

# 2. Opcionalmente parar MySQL (comentado por padrão)
# Descomente se quiser parar MySQL também
# echo "💾 Parar MySQL? (y/N)"
# read -n 1 -r
# echo
# if [[ $REPLY =~ ^[Yy]$ ]]; then
#     sudo systemctl stop mysql 2>/dev/null || sudo systemctl stop mariadb 2>/dev/null
#     echo -e "${GREEN}✅ MySQL parado${NC}"
# fi

echo "✨ Serviços parados"
