#!/bin/bash
# deploy_cie_dual.sh - Script de deployment para node_cie_dual
# Sistema Aguada IIoT - Cisterna Ilha do Engenho

set -e  # Exit on error

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Banner
echo -e "${BLUE}"
echo "╔════════════════════════════════════════════════╗"
echo "║   Aguada IIoT - node_cie_dual Deployment      ║"
echo "║   Cisterna Ilha do Engenho (2 Reservatórios)  ║"
echo "╚════════════════════════════════════════════════╝"
echo -e "${NC}"

# Diretórios
PROJECT_DIR="$HOME/firmware_aguada/firmware"
NODE_DIR="$PROJECT_DIR/node_cie_dual"

# Função para print com cores
print_step() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

# Verificar se estamos no diretório correto
if [ ! -d "$NODE_DIR" ]; then
    print_error "Diretório $NODE_DIR não encontrado!"
    exit 1
fi

cd "$NODE_DIR"
print_step "Navegado para: $NODE_DIR"

# Menu principal
echo ""
echo "Selecione uma opção:"
echo "  1) Build firmware"
echo "  2) Flash firmware"
echo "  3) Monitor serial"
echo "  4) Flash + Monitor"
echo "  5) Full clean + Build + Flash + Monitor"
echo "  6) Extrair MAC do ESP32"
echo "  7) Configurar backend SQL"
echo "  8) Verificar configuração backend"
echo "  9) Ver documentação"
echo "  0) Sair"
echo ""
read -p "Opção: " option

case $option in
    1)
        print_info "Compilando firmware..."
        idf.py build
        print_step "Build concluído!"
        ;;
    
    2)
        read -p "Porta serial (padrão: /dev/ttyUSB0): " port
        port=${port:-/dev/ttyUSB0}
        print_info "Flasheando em $port..."
        idf.py -p "$port" flash
        print_step "Flash concluído!"
        ;;
    
    3)
        read -p "Porta serial (padrão: /dev/ttyUSB0): " port
        port=${port:-/dev/ttyUSB0}
        print_info "Abrindo monitor serial em $port..."
        print_warning "Pressione Ctrl+] para sair"
        idf.py -p "$port" monitor --no-reset
        ;;
    
    4)
        read -p "Porta serial (padrão: /dev/ttyUSB0): " port
        port=${port:-/dev/ttyUSB0}
        print_info "Flash + Monitor em $port..."
        idf.py -p "$port" flash monitor
        ;;
    
    5)
        read -p "Porta serial (padrão: /dev/ttyUSB0): " port
        port=${port:-/dev/ttyUSB0}
        print_info "Full clean..."
        idf.py fullclean
        print_step "Clean concluído!"
        
        print_info "Compilando..."
        idf.py build
        print_step "Build concluído!"
        
        print_info "Flasheando..."
        idf.py -p "$port" flash
        print_step "Flash concluído!"
        
        print_info "Abrindo monitor..."
        print_warning "Pressione Ctrl+] para sair"
        idf.py -p "$port" monitor
        ;;
    
    6)
        read -p "Porta serial (padrão: /dev/ttyUSB0): " port
        port=${port:-/dev/ttyUSB0}
        print_info "Extraindo MAC do ESP32..."
        print_warning "Conecte o ESP32 e aguarde o boot..."
        echo ""
        
        # Capturar apenas a linha com o MAC
        idf.py -p "$port" monitor --no-reset 2>&1 | grep -m 1 "Device MAC" || true
        
        echo ""
        print_info "Anote o MAC acima e atualize backend_config.sql"
        print_info "Arquivo: $NODE_DIR/backend_config.sql (linha ~46)"
        ;;
    
    7)
        print_info "Configurando backend SQL..."
        
        # Verificar se backend_config.sql existe
        if [ ! -f "backend_config.sql" ]; then
            print_error "Arquivo backend_config.sql não encontrado!"
            exit 1
        fi
        
        # Pedir credenciais MySQL
        read -p "Usuário MySQL (padrão: root): " mysql_user
        mysql_user=${mysql_user:-root}
        
        read -sp "Senha MySQL: " mysql_pass
        echo ""
        
        read -p "Nome do banco (padrão: sensores_db): " mysql_db
        mysql_db=${mysql_db:-sensores_db}
        
        print_info "Executando SQL..."
        mysql -u "$mysql_user" -p"$mysql_pass" "$mysql_db" < backend_config.sql
        
        print_step "Backend configurado com sucesso!"
        print_info "Reservatórios CIE1 e CIE2 cadastrados"
        ;;
    
    8)
        print_info "Verificando configuração do backend..."
        
        read -p "Usuário MySQL (padrão: root): " mysql_user
        mysql_user=${mysql_user:-root}
        
        read -sp "Senha MySQL: " mysql_pass
        echo ""
        
        read -p "Nome do banco (padrão: sensores_db): " mysql_db
        mysql_db=${mysql_db:-sensores_db}
        
        echo ""
        print_info "Configurações de node_id=4 e node_id=5:"
        mysql -u "$mysql_user" -p"$mysql_pass" "$mysql_db" -e "SELECT * FROM node_configs WHERE node_id IN (4,5);"
        
        echo ""
        print_info "Últimas 5 leituras de cada sensor:"
        mysql -u "$mysql_user" -p"$mysql_pass" "$mysql_db" -e "
            (SELECT 'CIE1' AS sensor, node_id, level_cm, percentual, volume_l, FROM_UNIXTIME(ts_ms/1000) AS timestamp 
             FROM telemetry_processed WHERE node_id=4 ORDER BY ts_ms DESC LIMIT 5)
            UNION ALL
            (SELECT 'CIE2' AS sensor, node_id, level_cm, percentual, volume_l, FROM_UNIXTIME(ts_ms/1000) AS timestamp 
             FROM telemetry_processed WHERE node_id=5 ORDER BY ts_ms DESC LIMIT 5)
            ORDER BY timestamp DESC;
        " || print_warning "Ainda não há dados no banco (isso é normal antes do primeiro envio)"
        ;;
    
    9)
        print_info "Documentação disponível:"
        echo ""
        echo "  📄 README.md                    - Visão geral e pinout"
        echo "  📄 backend_config.sql           - Configuração SQL completa"
        echo "  📄 DASHBOARD_DESIGN.md          - Design do dashboard web"
        echo "  📄 IMPLEMENTATION_SUMMARY.md    - Resumo executivo completo"
        echo ""
        read -p "Abrir qual arquivo? (1-4 ou Enter para voltar): " doc_option
        
        case $doc_option in
            1) less README.md ;;
            2) less backend_config.sql ;;
            3) less DASHBOARD_DESIGN.md ;;
            4) less IMPLEMENTATION_SUMMARY.md ;;
            *) ;;
        esac
        ;;
    
    0)
        print_info "Saindo..."
        exit 0
        ;;
    
    *)
        print_error "Opção inválida!"
        exit 1
        ;;
esac

echo ""
print_step "Operação concluída!"
echo ""
print_info "Para mais informações, consulte:"
echo "  - README.md: Pinout e instruções básicas"
echo "  - IMPLEMENTATION_SUMMARY.md: Resumo executivo completo"
echo "  - DASHBOARD_DESIGN.md: Design do dashboard web"
echo ""
