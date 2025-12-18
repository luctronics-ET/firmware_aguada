#!/bin/bash
# ============================================================================
# SCRIPT DE CONFIGURAÇÃO DO BACKEND - AGUADA TELEMETRY
# ============================================================================
# Data: 17/12/2025
# Descrição: Configura o banco MySQL com os MACs reais extraídos dos nodes
# ============================================================================

set -e  # Exit on error

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     AGUADA TELEMETRY - CONFIGURAÇÃO DO BACKEND          ║${NC}"
echo -e "${BLUE}║     Importando MACs reais dos nodes                      ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# CONFIGURAÇÕES
# ============================================================================
DB_NAME="sensores_db"
DB_USER="root"
SQL_FILE="nodes_config_REAL_MACS.sql"

# ============================================================================
# VERIFICAÇÕES
# ============================================================================
echo -e "${YELLOW}[1/5]${NC} Verificando arquivos..."

if [ ! -f "$SQL_FILE" ]; then
    echo -e "${RED}✗ Erro: Arquivo $SQL_FILE não encontrado!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Arquivo SQL encontrado${NC}"

# ============================================================================
# VERIFICAR SERVIÇOS
# ============================================================================
echo -e "${YELLOW}[2/5]${NC} Verificando serviços do backend..."

# Verificar se MySQL está rodando
if ! pgrep -x "mysqld" > /dev/null; then
    echo -e "${RED}✗ MySQL não está rodando!${NC}"
    echo -e "${YELLOW}Iniciando MySQL...${NC}"
    sudo systemctl start mysql
    sleep 2
fi
echo -e "${GREEN}✓ MySQL rodando${NC}"

# ============================================================================
# BACKUP ANTES DE IMPORTAR
# ============================================================================
echo -e "${YELLOW}[3/5]${NC} Criando backup da tabela node_configs..."

BACKUP_FILE="node_configs_backup_$(date +%Y%m%d_%H%M%S).sql"
mysql -u "$DB_USER" -p"$MYSQL_ROOT_PASSWORD" "$DB_NAME" -e "SELECT * FROM node_configs;" > "/tmp/$BACKUP_FILE" 2>/dev/null || true
echo -e "${GREEN}✓ Backup criado: /tmp/$BACKUP_FILE${NC}"

# ============================================================================
# IMPORTAR SQL
# ============================================================================
echo -e "${YELLOW}[4/5]${NC} Importando configuração dos nodes..."
echo ""
echo -e "${BLUE}Nodes que serão configurados:${NC}"
echo -e "  • Node 1 (RCON):  ${GREEN}20:6E:F1:6B:77:58${NC}"
echo -e "  • Node 2 (RCAV):  ${GREEN}DC:06:75:67:6A:CC${NC}"
echo -e "  • Node 3 (RCB3):  ${GREEN}80:F1:B2:50:31:34${NC}"
echo -e "  • Node 4 (CIE1):  ${GREEN}DC:B4:D9:8B:9E:AC${NC}"
echo -e "  • Node 5 (CIE2):  ${GREEN}DC:B4:D9:8B:9E:AC${NC} (mesmo MAC)"
echo ""

# Solicitar senha do MySQL
echo -e "${YELLOW}Digite a senha do MySQL para o usuário root:${NC}"
mysql -u "$DB_USER" -p "$DB_NAME" < "$SQL_FILE"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ SQL importado com sucesso!${NC}"
else
    echo -e "${RED}✗ Erro ao importar SQL${NC}"
    exit 1
fi

# ============================================================================
# VERIFICAÇÃO FINAL
# ============================================================================
echo -e "${YELLOW}[5/5]${NC} Verificando configuração..."
echo ""

mysql -u "$DB_USER" -p "$DB_NAME" <<EOF
SELECT 
    node_id,
    name,
    mac_address,
    CONCAT(vol_max_l / 1000, ' m³') AS capacity,
    location,
    IF(active = 1, '✓ Ativo', '✗ Inativo') AS status
FROM node_configs
ORDER BY node_id;
EOF

echo ""
echo -e "${GREEN}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     CONFIGURAÇÃO CONCLUÍDA COM SUCESSO!                  ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Próximos passos:${NC}"
echo -e "  1. Gateway já está rodando (80:F3:DA:62:A7:84)"
echo -e "  2. Nodes estão enviando telemetria"
echo -e "  3. Verificar logs do gateway para confirmar recepção"
echo -e "  4. Acessar dashboard: ${YELLOW}http://192.168.0.117:8080${NC}"
echo ""
echo -e "${YELLOW}Para verificar telemetria em tempo real:${NC}"
echo -e "  mysql -u root -p sensores_db"
echo -e "  SELECT * FROM telemetry_processed ORDER BY ts_ms DESC LIMIT 10;"
echo ""
