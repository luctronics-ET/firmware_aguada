#!/bin/bash
# ============================================================================
# flash_all_interactive.sh - Menu interativo para gravar todos os nodes
# ============================================================================

set -e

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

BASE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
MAC_LOG="$BASE_DIR/macs_extraidos.txt"

# ============================================================================
# FUNÇÕES
# ============================================================================

print_header() {
    clear
    echo -e "${CYAN}"
    cat << "EOF"
╔═══════════════════════════════════════════════════════════════════════╗
║            🌊 AGUADA IoT - Flash de Nodes Automático 🌊              ║
║                     ESP32-C3 + HC-SR04 Telemetry                      ║
╚═══════════════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

show_status() {
    echo ""
    echo -e "${MAGENTA}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║                   STATUS DA GRAVAÇÃO                        ║${NC}"
    echo -e "${MAGENTA}╠═══════════════════════════════════════════════════════════════╣${NC}"
    
    for i in {1..4}; do
        if [ -f "$BASE_DIR/node${i}_mac.txt" ]; then
            mac=$(cat "$BASE_DIR/node${i}_mac.txt")
            echo -e "${MAGENTA}║${NC} Node $i ${GREEN}✅ GRAVADO${NC}   MAC: $mac"
        else
            echo -e "${MAGENTA}║${NC} Node $i ${RED}⚠️  PENDENTE${NC}"
        fi
    done
    
    echo -e "${MAGENTA}╠═══════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${MAGENTA}║${NC} Node 5 usa o MESMO ESP32 que Node 4 (CIE dual-sensor)   ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

show_menu() {
    print_header
    show_status
    
    echo -e "${CYAN}Escolha uma opção:${NC}"
    echo ""
    echo "  ${GREEN}[1]${NC} Flash Node 1 - RCON (Castelo Consumo)          80.000L"
    echo "  ${GREEN}[2]${NC} Flash Node 2 - RCAV (Castelo Incêndio)         80.000L"
    echo "  ${GREEN}[3]${NC} Flash Node 3 - RCB3 (Casa de Bombas 03)        80.000L"
    echo "  ${GREEN}[4]${NC} Flash Node 4/5 - CIE1+CIE2 (Cisternas)         245.000L × 2"
    echo ""
    echo "  ${YELLOW}[5]${NC} Flash TODOS os nodes sequencialmente"
    echo "  ${BLUE}[6]${NC} Mostrar MACs extraídos"
    echo "  ${BLUE}[7]${NC} Gerar SQL com MACs extraídos"
    echo "  ${BLUE}[8]${NC} Executar SQL no banco de dados"
    echo "  ${BLUE}[9]${NC} Verificar portas USB disponíveis"
    echo ""
    echo "  ${RED}[0]${NC} Sair"
    echo ""
    echo -n "Opção: "
}

detect_port() {
    # Tentar detectar porta USB automaticamente
    if [ -e /dev/ttyUSB0 ]; then
        echo "/dev/ttyUSB0"
    elif [ -e /dev/ttyACM0 ]; then
        echo "/dev/ttyACM0"
    elif [ -e /dev/ttyUSB1 ]; then
        echo "/dev/ttyUSB1"
    else
        echo ""
    fi
}

prompt_port() {
    local default_port=$(detect_port)
    
    echo ""
    if [ -n "$default_port" ]; then
        print_success "Porta detectada: $default_port"
        echo -n "Usar esta porta? [S/n]: "
        read -r use_default
        
        if [ "$use_default" == "n" ] || [ "$use_default" == "N" ]; then
            echo -n "Digite a porta serial: "
            read -r custom_port
            echo "$custom_port"
        else
            echo "$default_port"
        fi
    else
        print_warning "Nenhuma porta USB detectada automaticamente"
        echo ""
        ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "Nenhum dispositivo /dev/ttyUSB* ou /dev/ttyACM* encontrado"
        echo ""
        echo -n "Digite a porta serial: "
        read -r custom_port
        echo "$custom_port"
    fi
}

flash_node() {
    local node_num=$1
    
    print_header
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Gravando Node $node_num${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    local port=$(prompt_port)
    
    echo ""
    print_info "Conecte o ESP32 Node $node_num na porta $port"
    echo -n "Pressione ENTER quando estiver pronto..."
    read -r
    
    echo ""
    if "$BASE_DIR/flash_node.sh" "$node_num" "$port"; then
        print_success "Node $node_num gravado com sucesso!"
        
        # Registrar no log
        if [ -f "$BASE_DIR/node${node_num}_mac.txt" ]; then
            mac=$(cat "$BASE_DIR/node${node_num}_mac.txt")
            echo "Node $node_num: $mac" >> "$MAC_LOG"
        fi
    else
        print_error "Falha ao gravar Node $node_num"
    fi
    
    echo ""
    echo -n "Pressione ENTER para continuar..."
    read -r
}

flash_all_nodes() {
    print_header
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Gravando TODOS os nodes (1, 2, 3, 4)${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    print_warning "Você precisará conectar 4 ESP32-C3 sequencialmente"
    echo ""
    echo -n "Continuar? [S/n]: "
    read -r confirm
    
    if [ "$confirm" == "n" ] || [ "$confirm" == "N" ]; then
        return
    fi
    
    # Limpar log de MACs anterior
    > "$MAC_LOG"
    
    # Gravar nodes 1, 2, 3 (padrão)
    for i in {1..3}; do
        flash_node "$i"
    done
    
    # Gravar node 4 (CIE dual)
    flash_node 4
    
    print_success "Todos os nodes gravados!"
    echo ""
    echo -n "Pressione ENTER para continuar..."
    read -r
}

show_extracted_macs() {
    print_header
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  MACs Extraídos${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    local found=0
    for i in {1..4}; do
        if [ -f "$BASE_DIR/node${i}_mac.txt" ]; then
            mac=$(cat "$BASE_DIR/node${i}_mac.txt")
            echo -e "  ${GREEN}Node $i:${NC} $mac"
            found=1
        fi
    done
    
    if [ $found -eq 0 ]; then
        print_warning "Nenhum MAC extraído ainda"
        echo "Grave os nodes primeiro (opções 1-5)"
    fi
    
    echo ""
    if [ -f "$MAC_LOG" ]; then
        echo "Log completo: $MAC_LOG"
    fi
    
    echo ""
    echo -n "Pressione ENTER para continuar..."
    read -r
}

generate_sql() {
    print_header
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Gerando SQL com MACs Extraídos${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Verificar se todos os MACs foram extraídos
    local missing=0
    for i in {1..4}; do
        if [ ! -f "$BASE_DIR/node${i}_mac.txt" ]; then
            print_warning "Node $i ainda não foi gravado (MAC não extraído)"
            missing=1
        fi
    done
    
    if [ $missing -eq 1 ]; then
        print_error "Grave todos os nodes antes de gerar SQL"
        echo ""
        echo -n "Pressione ENTER para continuar..."
        read -r
        return
    fi
    
    # Ler MACs
    MAC1=$(cat "$BASE_DIR/node1_mac.txt")
    MAC2=$(cat "$BASE_DIR/node2_mac.txt")
    MAC3=$(cat "$BASE_DIR/node3_mac.txt")
    MAC4=$(cat "$BASE_DIR/node4_mac.txt")
    
    # Calcular MAC fictício para Node 5 (last byte +1)
    last_byte=$(echo "$MAC4" | awk -F: '{print $6}')
    next_byte=$(printf "%02X" $((0x$last_byte + 1)))
    MAC5=$(echo "$MAC4" | sed "s/:$last_byte$/:$next_byte/")
    
    # Gerar SQL unificado
    SQL_FILE="$BASE_DIR/nodes_all_config_GENERATED.sql"
    
    cat > "$SQL_FILE" << EOF
-- ============================================================================
-- Aguada IIoT - Configuração de Todos os Nodes
-- Gerado automaticamente em: $(date)
-- ============================================================================

USE sensores_db;

-- ============================================================================
-- NODES PADRÃO (80.000L) - Firmware: node_ultra1
-- ============================================================================

-- Node 1: RCON - Castelo de Consumo
INSERT INTO node_configs (mac, node_id, location, sensor_offset_cm, level_max_cm, vol_max_l)
VALUES ('$MAC1', 1, 'RCON - Castelo de Consumo', 20, 450, 80000)
ON DUPLICATE KEY UPDATE 
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l);

-- Node 2: RCAV - Castelo de Incêndio
INSERT INTO node_configs (mac, node_id, location, sensor_offset_cm, level_max_cm, vol_max_l)
VALUES ('$MAC2', 2, 'RCAV - Castelo de Incêndio', 20, 450, 80000)
ON DUPLICATE KEY UPDATE 
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l);

-- Node 3: RCB3 - Casa de Bombas 03
INSERT INTO node_configs (mac, node_id, location, sensor_offset_cm, level_max_cm, vol_max_l)
VALUES ('$MAC3', 3, 'RCB3 - Casa de Bombas 03', 20, 450, 80000)
ON DUPLICATE KEY UPDATE 
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l);

-- ============================================================================
-- CISTERNA CIE (245.000L) - Firmware: node_cie_dual
-- 1 ESP32 com 2 sensores HC-SR04
-- ============================================================================

-- Node 4: CIE1 - Cisterna Ilha Engenho 01 (MAC real)
INSERT INTO node_configs (mac, node_id, location, sensor_offset_cm, level_max_cm, vol_max_l)
VALUES ('$MAC4', 4, 'CIE1 - Cisterna Ilha Engenho 01', 20, 450, 245000)
ON DUPLICATE KEY UPDATE 
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l);

-- Node 5: CIE2 - Cisterna Ilha Engenho 02 (MAC fictício: last byte +1)
INSERT INTO node_configs (mac, node_id, location, sensor_offset_cm, level_max_cm, vol_max_l)
VALUES ('$MAC5', 5, 'CIE2 - Cisterna Ilha Engenho 02', 20, 450, 245000)
ON DUPLICATE KEY UPDATE 
    mac = VALUES(mac),
    location = VALUES(location),
    sensor_offset_cm = VALUES(sensor_offset_cm),
    level_max_cm = VALUES(level_max_cm),
    vol_max_l = VALUES(vol_max_l);

-- ============================================================================
-- VERIFICAÇÃO
-- ============================================================================

SELECT 
    node_id,
    mac,
    location,
    CONCAT(FORMAT(vol_max_l / 1000, 0), 'm³') AS capacidade,
    CONCAT(level_max_cm, 'cm') AS altura_max
FROM node_configs
ORDER BY node_id;

-- Capacidade total: 730.000L (730m³)
-- Nodes 1-3: 240.000L (80.000L cada)
-- Nodes 4-5: 490.000L (245.000L cada)
EOF

    print_success "SQL gerado: $SQL_FILE"
    echo ""
    echo "MACs configurados:"
    echo "  Node 1 (RCON): $MAC1"
    echo "  Node 2 (RCAV): $MAC2"
    echo "  Node 3 (RCB3): $MAC3"
    echo "  Node 4 (CIE1): $MAC4"
    echo "  Node 5 (CIE2): $MAC5 (fictício)"
    echo ""
    echo -n "Pressione ENTER para continuar..."
    read -r
}

execute_sql() {
    print_header
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Executar SQL no Banco de Dados${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    SQL_FILE="$BASE_DIR/nodes_all_config_GENERATED.sql"
    
    if [ ! -f "$SQL_FILE" ]; then
        print_error "Arquivo SQL não encontrado: $SQL_FILE"
        echo "Gere o SQL primeiro (opção 7)"
        echo ""
        echo -n "Pressione ENTER para continuar..."
        read -r
        return
    fi
    
    print_info "Arquivo SQL: $SQL_FILE"
    echo ""
    echo -n "Digite o usuário MySQL [root]: "
    read -r mysql_user
    mysql_user=${mysql_user:-root}
    
    echo -n "Digite o banco de dados [sensores_db]: "
    read -r mysql_db
    mysql_db=${mysql_db:-sensores_db}
    
    echo ""
    print_warning "Executando SQL..."
    
    if mysql -u "$mysql_user" -p "$mysql_db" < "$SQL_FILE"; then
        print_success "SQL executado com sucesso!"
        echo ""
        echo "Verificando configuração..."
        mysql -u "$mysql_user" -p "$mysql_db" -e "SELECT node_id, mac, location, vol_max_l FROM node_configs ORDER BY node_id;"
    else
        print_error "Falha ao executar SQL"
    fi
    
    echo ""
    echo -n "Pressione ENTER para continuar..."
    read -r
}

check_ports() {
    print_header
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Portas USB Disponíveis${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    if ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null; then
        echo ""
        print_success "Portas encontradas acima"
    else
        print_warning "Nenhuma porta USB detectada"
        echo ""
        print_info "Verifique:"
        echo "  1. ESP32 está conectado via USB"
        echo "  2. Driver CH340/CP2102 instalado"
        echo "  3. Usuário no grupo 'dialout': sudo usermod -a -G dialout \$USER"
    fi
    
    echo ""
    echo -n "Pressione ENTER para continuar..."
    read -r
}

# ============================================================================
# MAIN LOOP
# ============================================================================

while true; do
    show_menu
    read -r option
    
    case $option in
        1) flash_node 1 ;;
        2) flash_node 2 ;;
        3) flash_node 3 ;;
        4) flash_node 4 ;;
        5) flash_all_nodes ;;
        6) show_extracted_macs ;;
        7) generate_sql ;;
        8) execute_sql ;;
        9) check_ports ;;
        0)
            print_header
            echo ""
            print_success "Até logo!"
            echo ""
            exit 0
            ;;
        *)
            print_error "Opção inválida"
            sleep 1
            ;;
    esac
done
