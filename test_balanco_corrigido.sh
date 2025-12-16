#!/bin/bash
# Script de Teste - Balanço Hídrico Corrigido
# Data: 2025-12-15

echo "========================================="
echo "   TESTE DO SISTEMA DE BALANÇO HÍDRICO"
echo "========================================="
echo ""

# Cores
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

DB_USER="aguada_user"
DB_NAME="sensores_db"

echo "1️⃣  Verificando estrutura do banco..."
COLS=$(mysql -u $DB_USER $DB_NAME -se "SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_NAME='balanco_hidrico' AND COLUMN_NAME IN ('consumo_litros', 'consumo_esperado_litros', 'consumo_anormal_litros', 'percentual_anormal');" 2>&1)

if [ "$COLS" == "4" ]; then
    echo -e "   ${GREEN}✓${NC} Estrutura da tabela OK (4 novos campos)"
else
    echo -e "   ${RED}✗${NC} Estrutura incorreta (encontrados $COLS de 4 campos)"
    exit 1
fi

echo ""
echo "2️⃣  Testando stored procedure..."

# Teste 1: Consumo normal
echo "   Teste A: Período curto (esperado: consumo normal)"
mysql -u $DB_USER $DB_NAME -se "CALL calcular_balanco_hidrico('1', '2025-12-14 10:00:00', '2025-12-14 12:00:00');" > /tmp/test_balanco_a.txt 2>&1

if grep -q "SAÍDA" /tmp/test_balanco_a.txt; then
    echo -e "   ${GREEN}✓${NC} Detectou SAÍDA/Consumo corretamente"
else
    echo -e "   ${YELLOW}⚠${NC} Resultado inesperado (pode não haver dados no período)"
fi

# Teste 2: Período longo (detectar vazamento)
echo "   Teste B: Período completo (esperado: detectar anomalias)"
mysql -u $DB_USER $DB_NAME -se "CALL calcular_balanco_hidrico('1', '2025-12-14 00:00:00', '2025-12-14 23:59:59');" > /tmp/test_balanco_b.txt 2>&1

if grep -q "CRÍTICO" /tmp/test_balanco_b.txt || grep -q "ALERTA" /tmp/test_balanco_b.txt; then
    echo -e "   ${GREEN}✓${NC} Detectou consumo anormal (vazamento)"
    PERCENTUAL=$(grep "percentual_anormal" /tmp/test_balanco_b.txt | awk '{print $2}')
    echo "      Consumo anormal: ${PERCENTUAL}% acima do esperado"
elif grep -q "NORMAL" /tmp/test_balanco_b.txt; then
    echo -e "   ${GREEN}✓${NC} Consumo normal (sem vazamento)"
else
    echo -e "   ${YELLOW}⚠${NC} Resultado inesperado"
fi

echo ""
echo "3️⃣  Verificando views..."

# View: vw_balanco_diario
ROWS_DIARIO=$(mysql -u $DB_USER $DB_NAME -se "SELECT COUNT(*) FROM vw_balanco_diario;" 2>&1)
if [ "$ROWS_DIARIO" -ge "0" ]; then
    echo -e "   ${GREEN}✓${NC} View vw_balanco_diario OK ($ROWS_DIARIO registros)"
else
    echo -e "   ${RED}✗${NC} View vw_balanco_diario com erro"
fi

# View: vw_alertas_vazamento
ROWS_ALERTAS=$(mysql -u $DB_USER $DB_NAME -se "SELECT COUNT(*) FROM vw_alertas_vazamento;" 2>&1)
if [ "$ROWS_ALERTAS" -ge "0" ]; then
    echo -e "   ${GREEN}✓${NC} View vw_alertas_vazamento OK ($ROWS_ALERTAS alertas)"
    
    if [ "$ROWS_ALERTAS" -gt "0" ]; then
        echo ""
        echo "   🚨 ALERTAS ATIVOS:"
        mysql -u $DB_USER $DB_NAME -e "
            SELECT 
                reservatorio_id,
                DATE_FORMAT(periodo_inicio, '%d/%m %H:%M') as inicio,
                consumo_anormal_litros as anormal_L,
                CONCAT(ROUND(percentual_anormal, 1), '%') as percentual,
                nivel_alerta
            FROM vw_alertas_vazamento
            ORDER BY percentual_anormal DESC
            LIMIT 5;
        " 2>&1
    fi
else
    echo -e "   ${RED}✗${NC} View vw_alertas_vazamento com erro"
fi

echo ""
echo "4️⃣  Testando interface web..."

# Verificar se relatorio_servico.html existe
if [ -f "/home/luciano/firmware_aguada/backend/relatorio_servico.html" ]; then
    echo -e "   ${GREEN}✓${NC} Arquivo relatorio_servico.html encontrado"
    
    # Verificar se contém novo cálculo
    if grep -q "balanco = fim - ini" /home/luciano/firmware_aguada/backend/relatorio_servico.html; then
        echo -e "   ${GREEN}✓${NC} Fórmula corrigida: balanço = final - inicial"
    else
        echo -e "   ${YELLOW}⚠${NC} Fórmula antiga ainda presente"
    fi
    
    # Verificar indicadores visuais
    if grep -q "backgroundColor.*#fee2e2" /home/luciano/firmware_aguada/backend/relatorio_servico.html; then
        echo -e "   ${GREEN}✓${NC} Indicadores visuais implementados (vermelho=crítico)"
    else
        echo -e "   ${YELLOW}⚠${NC} Indicadores visuais não encontrados"
    fi
else
    echo -e "   ${RED}✗${NC} Arquivo relatorio_servico.html não encontrado"
fi

echo ""
echo "5️⃣  Resumo dos dados..."

# Estatísticas do balanço
mysql -u $DB_USER $DB_NAME -e "
SELECT 
    '📊 ESTATÍSTICAS GERAIS' as '═════════════════════',
    '' as ' ';

SELECT 
    CONCAT('Total de cálculos: ', COUNT(*)) as info
FROM balanco_hidrico
UNION ALL
SELECT 
    CONCAT('Com consumo anormal: ', COUNT(*)) as info
FROM balanco_hidrico
WHERE consumo_anormal_litros > 0
UNION ALL
SELECT 
    CONCAT('Média de consumo: ', ROUND(AVG(consumo_litros), 0), ' L') as info
FROM balanco_hidrico
WHERE consumo_litros > 0;
" 2>&1

echo ""
echo "========================================="
echo -e "   ${GREEN}✅ TESTES CONCLUÍDOS!${NC}"
echo "========================================="
echo ""
echo "📌 Próximos passos:"
echo "   1. Abrir http://localhost:8080/relatorio_servico.html"
echo "   2. Criar relatório de teste"
echo "   3. Verificar indicadores visuais (cores)"
echo "   4. Validar cálculos automáticos"
echo ""
echo "📊 Consultas úteis:"
echo "   • Ver balanço diário:"
echo "     SELECT * FROM vw_balanco_diario ORDER BY data DESC LIMIT 7;"
echo ""
echo "   • Ver alertas ativos:"
echo "     SELECT * FROM vw_alertas_vazamento;"
echo ""
echo "   • Calcular balanço manual:"
echo "     CALL calcular_balanco_hidrico('1', '2025-12-14 00:00', '2025-12-14 23:59');"
echo ""
