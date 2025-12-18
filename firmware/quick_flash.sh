#!/bin/bash
# Quick Flash Script - Sistema Aguada

set -e

echo "🔥 Sistema Aguada - Flash Rápido"
echo "================================"
echo ""

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para detectar dispositivos
detect_devices() {
    echo -e "${BLUE}📡 Detectando dispositivos conectados...${NC}"
    echo ""
    
    if ls /dev/ttyUSB* 2>/dev/null; then
        echo -e "${GREEN}✓ Encontrados dispositivos USB${NC}"
    fi
    
    if ls /dev/ttyACM* 2>/dev/null; then
        echo -e "${GREEN}✓ Encontrados dispositivos ACM${NC}"
    fi
    
    echo ""
}

# Menu principal
show_menu() {
    echo -e "${YELLOW}Escolha o que deseja gravar:${NC}"
    echo ""
    echo "1) Gateway 1 (ESP32 DevKit V1)"
    echo "2) Gateway 2 (ESP32 DevKit V1) - Redundância"
    echo "3) Node Ultra 1 (ESP32-C3 Supermini)"
    echo "4) Node Ultra 2 (ESP32-C3 Supermini)"
    echo "5) Detectar dispositivos novamente"
    echo "6) Limpar todos os builds"
    echo "0) Sair"
    echo ""
    read -p "Opção: " choice
    return $choice
}

# Flash Gateway
flash_gateway() {
    local port=$1
    local gateway_num=$2
    
    echo -e "${BLUE}📝 Gravando Gateway ${gateway_num}...${NC}"
    cd ~/firmware_aguada/firmware/gateway_devkit_v1
    
    echo -e "${YELLOW}Compilando...${NC}"
    idf.py build
    
    echo -e "${YELLOW}Gravando em ${port}...${NC}"
    idf.py -p ${port} flash
    
    echo -e "${GREEN}✓ Gateway ${gateway_num} gravado!${NC}"
    echo ""
    echo -e "${YELLOW}⚠️  IMPORTANTE: Anote o MAC address mostrado nos logs!${NC}"
    echo -e "${YELLOW}Iniciando monitor (Ctrl+] para sair)...${NC}"
    sleep 2
    idf.py -p ${port} monitor
}

# Flash Node
flash_node() {
    local port=$1
    local node_num=$2
    
    echo -e "${BLUE}📝 Gravando Node Ultra ${node_num}...${NC}"
    cd ~/firmware_aguada/firmware/node_ultra${node_num}
    
    echo -e "${YELLOW}Compilando...${NC}"
    idf.py build
    
    echo -e "${YELLOW}Gravando em ${port}...${NC}"
    idf.py -p ${port} flash
    
    echo -e "${GREEN}✓ Node Ultra ${node_num} gravado!${NC}"
    echo -e "${YELLOW}Iniciando monitor (Ctrl+] para sair)...${NC}"
    sleep 2
    idf.py -p ${port} monitor
}

# Limpar builds
clean_all() {
    echo -e "${YELLOW}🧹 Limpando todos os builds...${NC}"
    cd ~/firmware_aguada/firmware
    
    if [ -f "./limpar_builds.sh" ]; then
        ./limpar_builds.sh
    else
        rm -rf gateway_devkit_v1/build
        rm -rf node_ultra1/build
        rm -rf node_ultra2/build
        echo -e "${GREEN}✓ Builds limpos!${NC}"
    fi
}

# Loop principal
while true; do
    detect_devices
    show_menu
    choice=$?
    
    case $choice in
        1)
            read -p "Porta do Gateway 1 [/dev/ttyUSB0]: " port
            port=${port:-/dev/ttyUSB0}
            flash_gateway "$port" 1
            ;;
        2)
            read -p "Porta do Gateway 2 [/dev/ttyUSB1]: " port
            port=${port:-/dev/ttyUSB1}
            flash_gateway "$port" 2
            ;;
        3)
            read -p "Porta do Node Ultra 1 [/dev/ttyACM0]: " port
            port=${port:-/dev/ttyACM0}
            flash_node "$port" 1
            ;;
        4)
            read -p "Porta do Node Ultra 2 [/dev/ttyACM1]: " port
            port=${port:-/dev/ttyACM1}
            flash_node "$port" 2
            ;;
        5)
            continue
            ;;
        6)
            clean_all
            read -p "Pressione Enter para continuar..."
            ;;
        0)
            echo -e "${GREEN}👋 Até logo!${NC}"
            exit 0
            ;;
        *)
            echo -e "${RED}❌ Opção inválida!${NC}"
            sleep 2
            ;;
    esac
done
