#!/bin/bash
# Script para verificar status dos serviços

echo "📊 Aguada Telemetry - Status dos Serviços"
echo "=========================================="
echo ""

# Cores
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# MySQL
echo "💾 MySQL/MariaDB:"
if systemctl is-active --quiet mysql 2>/dev/null; then
    echo -e "   ${GREEN}✅ Rodando (mysql)${NC}"
elif systemctl is-active --quiet mariadb 2>/dev/null; then
    echo -e "   ${GREEN}✅ Rodando (mariadb)${NC}"
else
    echo -e "   ${RED}❌ Parado${NC}"
fi

# Banco de dados
if sudo mysql -e "USE sensores_db;" 2>/dev/null; then
    echo -e "   ${GREEN}✅ Banco 'sensores_db' existe${NC}"
    
    # Contar registros
    COUNT=$(sudo mysql sensores_db -s -N -e "SELECT COUNT(*) FROM leituras_v2;" 2>/dev/null || echo "0")
    echo "   📊 Registros na tabela: $COUNT"
else
    echo -e "   ${YELLOW}⚠️  Banco 'sensores_db' não encontrado${NC}"
fi
echo ""

# Servidor PHP
echo "🌐 Servidor PHP (porta 8080):"
if pgrep -f "php -S.*8080" > /dev/null; then
    PID=$(pgrep -f "php -S.*8080")
    echo -e "   ${GREEN}✅ Rodando (PID: $PID)${NC}"
    
    # Testar conectividade
    if curl -s http://localhost:8080 > /dev/null; then
        echo -e "   ${GREEN}✅ Respondendo em http://localhost:8080${NC}"
    else
        echo -e "   ${YELLOW}⚠️  Não está respondendo${NC}"
    fi
else
    echo -e "   ${RED}❌ Não está rodando${NC}"
fi
echo ""

# Gateway (verificar se há porta serial conectada)
echo "📡 Gateway ESP32:"
if ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1 > /dev/null; then
    DEVICES=$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | tr '\n' ' ')
    echo -e "   ${GREEN}✅ Dispositivos detectados: $DEVICES${NC}"
else
    echo -e "   ${YELLOW}⚠️  Nenhum dispositivo serial detectado${NC}"
fi
echo ""

# Últimas leituras (se banco estiver ativo)
if sudo mysql sensores_db -e "SELECT 1 FROM leituras_v2 LIMIT 1;" 2>/dev/null > /dev/null; then
    echo "📈 Últimas Leituras (por nó):"
    sudo mysql sensores_db -e "
        SELECT 
            node_id as 'Nó',
            COUNT(*) as 'Total',
            MAX(created_at) as 'Última Leitura',
            AVG(level_cm) as 'Nível Médio (cm)',
            AVG(rssi) as 'RSSI Médio'
        FROM leituras_v2 
        GROUP BY node_id 
        ORDER BY node_id;
    " 2>/dev/null || echo "   Nenhum dado ainda"
fi
echo ""

echo "=========================================="
echo "💡 Comandos úteis:"
echo "   • Iniciar:  ./start_services.sh"
echo "   • Parar:    ./stop_services.sh"
echo "   • Logs:     tail -f server.log"
echo "   • Monitor:  watch -n 2 ./status_services.sh"
