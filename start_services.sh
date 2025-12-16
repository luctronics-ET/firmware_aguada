#!/bin/bash
# Script de inicialização automática do Aguada Telemetry System

set -e  # Exit on error

echo "🌊 Aguada Telemetry - Inicialização Automática"
echo "=============================================="
echo ""

# Cores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Diretório do projeto
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$PROJECT_DIR"

# Função para verificar se comando existe
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 1. Verificar dependências
echo "📋 Verificando dependências..."
MYSQL_AVAILABLE=0
PHP_AVAILABLE=0

if command_exists mysql; then
    MYSQL_AVAILABLE=1
    echo -e "${GREEN}✅ MySQL/MariaDB encontrado${NC}"
else
    echo -e "${YELLOW}⚠️  MySQL/MariaDB não encontrado (modo demo)${NC}"
fi

if command_exists php; then
    PHP_AVAILABLE=1
    echo -e "${GREEN}✅ PHP encontrado${NC}"
else
    echo -e "${RED}❌ PHP não encontrado${NC}"
    echo ""
    echo "Instale PHP:"
    echo "  sudo apt install php php-mysql"
    exit 1
fi

echo ""

# 2. Verificar/Iniciar MySQL (se disponível)
if [ $MYSQL_AVAILABLE -eq 1 ]; then
    echo "💾 Verificando MySQL..."
    if systemctl is-active --quiet mysql 2>/dev/null || systemctl is-active --quiet mariadb 2>/dev/null; then
        echo -e "${GREEN}✅ MySQL já está rodando${NC}"
        MYSQL_RUNNING=1
    else
        echo -e "${YELLOW}⚠️  MySQL não está rodando${NC}"
        echo "   Para iniciar: sudo systemctl start mysql"
        echo "   Para instalar: sudo apt install mysql-server"
        MYSQL_RUNNING=0
    fi
    echo ""

    # 3. Configurar banco de dados
    if [ $MYSQL_RUNNING -eq 1 ]; then
        echo "🗄️  Configurando banco de dados..."
        
        # Verificar se banco existe
        if mysql -u root -e "USE sensores_db;" 2>/dev/null; then
            echo -e "${GREEN}✅ Banco 'sensores_db' já existe${NC}"
        else
            echo -e "${YELLOW}⚙️  Banco não encontrado${NC}"
            echo "   Execute: mysql -u root -p -e \"CREATE DATABASE sensores_db;\""
            echo "            mysql -u root -p sensores_db < database/schema.sql"
        fi

        # Verificar se tabela existe
        if mysql -u root sensores_db -e "SHOW TABLES LIKE 'leituras_v2';" 2>/dev/null | grep -q leituras_v2; then
            echo -e "${GREEN}✅ Tabela 'leituras_v2' já existe${NC}"
        fi
        echo ""
    fi
else
    echo -e "${YELLOW}⚠️  MySQL não instalado - rodando em modo DEMO${NC}"
    echo "   Para instalar: sudo apt install mysql-server"
    echo ""
fi

# 4. Parar servidor PHP antigo (se existir)
echo "🔧 Verificando servidor PHP..."
if pgrep -f "php -S.*8080" > /dev/null; then
    echo -e "${YELLOW}⚙️  Parando servidor PHP antigo...${NC}"
    pkill -f "php -S.*8080"
    sleep 1
fi

# 5. Iniciar servidor PHP
echo -e "${YELLOW}⚙️  Iniciando servidor PHP na porta 8080...${NC}"
cd backend
php -S 0.0.0.0:8080 > ../server.log 2>&1 &
PHP_PID=$!
cd ..

# Aguardar servidor iniciar
sleep 2

# Verificar se servidor está respondendo
if curl -s http://localhost:8080 > /dev/null; then
    echo -e "${GREEN}✅ Servidor PHP rodando (PID: $PHP_PID)${NC}"
else
    echo -e "${RED}❌ Servidor não está respondendo${NC}"
    exit 1
fi
echo ""

# 6. Resumo
echo "=============================================="
echo -e "${GREEN}🎉 Sistema inicializado com sucesso!${NC}"
echo "=============================================="
echo ""
echo "📊 Status dos Serviços:"
echo "  • MySQL:  $(systemctl is-active mysql 2>/dev/null || systemctl is-active mariadb 2>/dev/null || echo 'unknown')"
echo "  • Backend PHP: Rodando (PID: $PHP_PID)"
echo ""
echo "🌐 URLs Disponíveis:"
echo "  • Home:      http://localhost:8080"
echo "  • Dashboard: http://localhost:8080/dashboard.php"
echo "  • Ingest:    http://localhost:8080/ingest_sensorpacket.php"
echo ""
echo "📝 Logs:"
echo "  • Server:    tail -f $PROJECT_DIR/server.log"
echo ""
echo "🛑 Para parar:"
echo "  • ./stop_services.sh"
echo "  • ou: kill $PHP_PID"
echo ""

# Salvar PID para script de parada
echo $PHP_PID > .php_server.pid

# Abrir navegador (opcional)
if command_exists xdg-open; then
    echo "🌐 Abrindo navegador..."
    xdg-open http://localhost:8080 2>/dev/null &
fi
