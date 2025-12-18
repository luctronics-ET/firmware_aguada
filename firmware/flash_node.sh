#!/bin/bash
# ============================================================================
# flash_node.sh - Script para gravar firmware em nodes Aguada IIoT
# ============================================================================
# Uso: ./flash_node.sh <node_number> [porta]
# Exemplo: ./flash_node.sh 1 /dev/ttyUSB0
# ============================================================================

set -e  # Exit on error

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# CONFIGURAÇÃO
# ============================================================================

# Diretório base do projeto
BASE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Configuração dos nodes
declare -A NODE_NAMES=(
    [1]="RCON - Castelo de Consumo"
    [2]="RCAV - Castelo de Incêndio"
    [3]="RCB3 - Casa de Bombas 03"
    [4]="CIE1 - Cisterna Ilha Engenho 01"
    [5]="CIE2 - Cisterna Ilha Engenho 02"
)

declare -A NODE_FIRMWARE=(
    [1]="node_ultra1"
    [2]="node_ultra1"
    [3]="node_ultra1"
    [4]="node_cie_dual"
    [5]="node_cie_dual"
)

declare -A NODE_TARGET=(
    [1]="esp32c3"
    [2]="esp32c3"
    [3]="esp32c3"
    [4]="esp32c3"
    [5]="esp32c3"
)

# ============================================================================
# FUNÇÕES
# ============================================================================

print_header() {
    echo -e "${BLUE}============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}============================================================================${NC}"
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

show_usage() {
    cat << EOF
Uso: $0 <node_number> [porta]

Argumentos:
    node_number    Número do node (1-5)
    porta          Porta serial (default: /dev/ttyUSB0)

Nodes disponíveis:
    1 - RCON (Castelo de Consumo)       - firmware: node_ultra1
    2 - RCAV (Castelo de Incêndio)      - firmware: node_ultra1
    3 - RCB3 (Casa de Bombas 03)        - firmware: node_ultra1
    4 - CIE1 (Cisterna Engenho 01)      - firmware: node_cie_dual
    5 - CIE2 (Cisterna Engenho 02)      - firmware: node_cie_dual (mesmo ESP32 que node 4)

Exemplos:
    $0 1                    # Flash node 1 em /dev/ttyUSB0
    $0 2 /dev/ttyACM0       # Flash node 2 em /dev/ttyACM0
    $0 4                    # Flash node 4/5 (CIE dual-sensor)

Notas:
    - Nodes 1, 2, 3 usam firmware node_ultra1 (mudar NODE_ID antes de compilar)
    - Nodes 4 e 5 usam firmware node_cie_dual (1 ESP32 com 2 sensores)
    - Script automaticamente:
      * Configura target ESP32-C3
      * Compila firmware
      * Grava no ESP32
      * Extrai MAC address
      * Mostra instruções para atualizar SQL
EOF
}

check_port() {
    local port=$1
    if [ ! -e "$port" ]; then
        print_error "Porta $port não encontrada!"
        echo ""
        print_info "Portas disponíveis:"
        ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  Nenhuma porta USB detectada"
        echo ""
        print_info "Verifique se:"
        echo "  1. ESP32 está conectado via USB"
        echo "  2. Driver CH340/CP2102 está instalado"
        echo "  3. Usuário pertence ao grupo 'dialout': sudo usermod -a -G dialout \$USER"
        exit 1
    fi
    print_success "Porta $port encontrada"
}

check_esp_idf() {
    if ! command -v idf.py &> /dev/null; then
        print_error "ESP-IDF não encontrado!"
        echo ""
        print_info "Execute:"
        echo "  . \$HOME/esp/esp-idf/export.sh"
        exit 1
    fi
    print_success "ESP-IDF encontrado: $(idf.py --version 2>&1 | head -n1)"
}

update_node_id() {
    local node_num=$1
    local firmware_dir=$2
    local cpp_file="$firmware_dir/main/node_ultra1.cpp"
    
    # Nodes 4 e 5 (CIE) não precisam editar NODE_ID (já vem correto no firmware)
    if [ "$node_num" -eq 4 ] || [ "$node_num" -eq 5 ]; then
        print_info "Node CIE dual-sensor: NODE_ID já configurado no firmware"
        return 0
    fi
    
    if [ ! -f "$cpp_file" ]; then
        print_error "Arquivo não encontrado: $cpp_file"
        exit 1
    fi
    
    print_info "Atualizando NODE_ID para $node_num..."
    
    # Backup do arquivo original
    cp "$cpp_file" "$cpp_file.bak"
    
    # Substituir NODE_ID (assume linha tipo: static const uint8_t NODE_ID = X;)
    sed -i "s/static const uint8_t NODE_ID = [0-9]\+;/static const uint8_t NODE_ID = $node_num;/" "$cpp_file"
    
    # Verificar se substituição foi bem-sucedida
    if grep -q "NODE_ID = $node_num;" "$cpp_file"; then
        print_success "NODE_ID atualizado para $node_num"
    else
        print_error "Falha ao atualizar NODE_ID"
        # Restaurar backup
        mv "$cpp_file.bak" "$cpp_file"
        exit 1
    fi
}

extract_mac() {
    local port=$1
    local timeout=10
    
    print_info "Extraindo MAC address..."
    print_info "Pressione RESET no ESP32 se não aparecer nada em 5 segundos"
    
    # Capturar logs e procurar por MAC
    timeout $timeout idf.py -p "$port" monitor --no-reset 2>&1 | \
        grep -m 1 "Device MAC:" | \
        awk '{print $NF}' || echo "ERRO_MAC_NAO_ENCONTRADO"
}

show_sql_instructions() {
    local node_num=$1
    local mac=$2
    local node_name="${NODE_NAMES[$node_num]}"
    
    echo ""
    print_header "📝 PRÓXIMOS PASSOS"
    
    if [ "$node_num" -le 3 ]; then
        # Nodes padrão (1, 2, 3)
        echo "1. Editar arquivo SQL:"
        echo "   nano $BASE_DIR/node_ultra1/nodes_config.sql"
        echo ""
        echo "2. Substituir placeholder do Node $node_num:"
        if [ "$node_num" -eq 1 ]; then
            echo "   'C8:2B:96:XX:XX:XX' → '$mac'"
        elif [ "$node_num" -eq 2 ]; then
            echo "   'C8:2B:96:YY:YY:YY' → '$mac'"
        elif [ "$node_num" -eq 3 ]; then
            echo "   'C8:2B:96:ZZ:ZZ:ZZ' → '$mac'"
        fi
    else
        # Nodes CIE (4, 5)
        echo "1. Editar arquivo SQL:"
        echo "   nano $BASE_DIR/node_cie_dual/backend_config.sql"
        echo ""
        echo "2. Substituir MACs (mesmo ESP32 envia 2 pacotes):"
        echo "   Node 4 (CIE1): 'C8:2B:96:XX:XX:XX' → '$mac' (MAC real)"
        
        # Calcular MAC fictício (last byte +1)
        last_byte=$(echo "$mac" | awk -F: '{print $6}')
        next_byte=$(printf "%02X" $((0x$last_byte + 1)))
        fake_mac=$(echo "$mac" | sed "s/:$last_byte$/:$next_byte/")
        
        echo "   Node 5 (CIE2): 'C8:2B:96:XX:XX:XY' → '$fake_mac' (MAC fictício)"
    fi
    
    echo ""
    echo "3. Executar SQL:"
    if [ "$node_num" -le 3 ]; then
        echo "   mysql -u root -p sensores_db < $BASE_DIR/node_ultra1/nodes_config.sql"
    else
        echo "   mysql -u root -p sensores_db < $BASE_DIR/node_cie_dual/backend_config.sql"
    fi
    
    echo ""
    print_success "Node $node_num ($node_name) gravado com sucesso!"
}

# ============================================================================
# MAIN
# ============================================================================

# Verificar argumentos
if [ $# -lt 1 ]; then
    show_usage
    exit 1
fi

NODE_NUM=$1
PORT=${2:-/dev/ttyUSB0}

# Validar node number
if [ "$NODE_NUM" -lt 1 ] || [ "$NODE_NUM" -gt 5 ]; then
    print_error "Node number inválido: $NODE_NUM"
    echo "Use um número entre 1 e 5"
    exit 1
fi

# Nodes 4 e 5 compartilham o mesmo firmware
if [ "$NODE_NUM" -eq 5 ]; then
    print_warning "Node 5 (CIE2) usa o MESMO ESP32 que Node 4 (CIE1)"
    print_info "Grave apenas o Node 4 (firmware node_cie_dual)"
    print_info "O ESP32 enviará 2 pacotes automaticamente (node_id=4 e node_id=5)"
    exit 0
fi

NODE_NAME="${NODE_NAMES[$NODE_NUM]}"
FIRMWARE="${NODE_FIRMWARE[$NODE_NUM]}"
TARGET="${NODE_TARGET[$NODE_NUM]}"
FIRMWARE_DIR="$BASE_DIR/$FIRMWARE"

# Header
print_header "🔥 Flash Node $NODE_NUM - $NODE_NAME"
echo ""
print_info "Firmware: $FIRMWARE"
print_info "Target: $TARGET"
print_info "Porta: $PORT"
echo ""

# Verificações
check_esp_idf
check_port "$PORT"

# Mudar para diretório do firmware
cd "$FIRMWARE_DIR" || exit 1

# Atualizar NODE_ID (apenas para nodes 1, 2, 3)
update_node_id "$NODE_NUM" "$FIRMWARE_DIR"

# Set target
print_info "Configurando target $TARGET..."
idf.py set-target "$TARGET" > /dev/null 2>&1 || true
print_success "Target configurado"

# Compilar
print_info "Compilando firmware..."
if idf.py build; then
    print_success "Compilação concluída"
else
    print_error "Falha na compilação"
    exit 1
fi

# Gravar
print_info "Gravando firmware em $PORT..."
if idf.py -p "$PORT" flash; then
    print_success "Firmware gravado"
else
    print_error "Falha ao gravar firmware"
    exit 1
fi

# Extrair MAC
MAC=$(extract_mac "$PORT")

if [ "$MAC" == "ERRO_MAC_NAO_ENCONTRADO" ] || [ -z "$MAC" ]; then
    print_error "Não foi possível extrair MAC automaticamente"
    echo ""
    print_info "Execute manualmente:"
    echo "  idf.py -p $PORT monitor"
    echo ""
    echo "Procure por linha semelhante a:"
    echo "  I (500) node_ultra01: Device MAC: C8:2B:96:XX:XX:XX"
    exit 1
else
    print_success "MAC extraído: $MAC"
    
    # Salvar MAC em arquivo
    MAC_FILE="$BASE_DIR/node${NODE_NUM}_mac.txt"
    echo "$MAC" > "$MAC_FILE"
    print_success "MAC salvo em: $MAC_FILE"
fi

# Mostrar instruções SQL
show_sql_instructions "$NODE_NUM" "$MAC"

echo ""
print_info "Desconecte o ESP32 e conecte o próximo para gravar"
echo ""
