#!/bin/bash
# Teste rápido do sistema de auto-start AGUADA

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}"
echo "╔════════════════════════════════════════╗"
echo "║   TESTE DO SISTEMA AUTO-START AGUADA  ║"
echo "╚════════════════════════════════════════╝"
echo -e "${NC}"

PROJECT_DIR="/home/luciano/firmware_aguada"

# Teste 1: Scripts existem?
echo -e "\n${YELLOW}[1]${NC} Verificando scripts..."
if [ -f "$PROJECT_DIR/autostart_gateway.sh" ] && [ -x "$PROJECT_DIR/autostart_gateway.sh" ]; then
    echo -e "  ${GREEN}✓${NC} autostart_gateway.sh (executável)"
else
    echo -e "  ${RED}✗${NC} autostart_gateway.sh (problema)"
fi

if [ -f "$PROJECT_DIR/install_aguada.sh" ] && [ -x "$PROJECT_DIR/install_aguada.sh" ]; then
    echo -e "  ${GREEN}✓${NC} install_aguada.sh (executável)"
else
    echo -e "  ${RED}✗${NC} install_aguada.sh (problema)"
fi

# Teste 2: Dependências instaladas?
echo -e "\n${YELLOW}[2]${NC} Verificando dependências..."

check_cmd() {
    if command -v $1 &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $1"
        return 0
    else
        echo -e "  ${RED}✗${NC} $1 (não instalado)"
        return 1
    fi
}

check_cmd git
check_cmd php
check_cmd mysql
check_cmd python3

# ESP-IDF
if [ -d "$HOME/esp/esp-idf" ]; then
    echo -e "  ${GREEN}✓${NC} ESP-IDF ($HOME/esp/esp-idf)"
else
    echo -e "  ${YELLOW}!${NC} ESP-IDF (não instalado)"
fi

# Teste 3: MySQL rodando?
echo -e "\n${YELLOW}[3]${NC} Verificando MySQL..."
if systemctl is-active --quiet mysql; then
    echo -e "  ${GREEN}✓${NC} MySQL ativo"
    
    # Testar conexão
    if mysql -u aguada_user -e "USE sensores_db" 2>/dev/null; then
        echo -e "  ${GREEN}✓${NC} Banco 'sensores_db' acessível"
    else
        echo -e "  ${RED}✗${NC} Banco 'sensores_db' (não configurado)"
    fi
else
    echo -e "  ${RED}✗${NC} MySQL não está rodando"
fi

# Teste 4: Backend funcionando?
echo -e "\n${YELLOW}[4]${NC} Verificando backend..."
if curl -s http://localhost:8080 > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} Backend PHP respondendo na porta 8080"
else
    echo -e "  ${YELLOW}!${NC} Backend não está rodando (normal se não iniciado)"
fi

# Teste 5: Dispositivos USB
echo -e "\n${YELLOW}[5]${NC} Verificando dispositivos USB..."
usb_found=false
for port in /dev/ttyUSB* /dev/ttyACM*; do
    if [ -e "$port" ]; then
        echo -e "  ${GREEN}✓${NC} $port detectado"
        usb_found=true
    fi
done

if [ "$usb_found" = false ]; then
    echo -e "  ${YELLOW}!${NC} Nenhum dispositivo USB detectado"
fi

# Teste 6: Permissões USB
echo -e "\n${YELLOW}[6]${NC} Verificando permissões USB..."
if groups | grep -q dialout; then
    echo -e "  ${GREEN}✓${NC} Usuário no grupo 'dialout'"
else
    echo -e "  ${RED}✗${NC} Usuário NÃO está no grupo 'dialout'"
    echo -e "  ${YELLOW}→${NC} Execute: ${GREEN}sudo usermod -a -G dialout \$USER${NC}"
    echo -e "  ${YELLOW}→${NC} Depois faça logout e login"
fi

# Teste 7: Regras udev
echo -e "\n${YELLOW}[7]${NC} Verificando regras udev..."
if [ -f /etc/udev/rules.d/99-aguada-gateway.rules ]; then
    echo -e "  ${GREEN}✓${NC} Regra udev instalada"
else
    echo -e "  ${YELLOW}!${NC} Regra udev não instalada (auto-start não funciona)"
    echo -e "  ${YELLOW}→${NC} Execute: ${GREEN}./install_aguada.sh${NC}"
fi

# Teste 8: Serviço systemd
echo -e "\n${YELLOW}[8]${NC} Verificando serviço systemd..."
if [ -f /etc/systemd/system/aguada-autostart@.service ]; then
    echo -e "  ${GREEN}✓${NC} Serviço systemd instalado"
else
    echo -e "  ${YELLOW}!${NC} Serviço systemd não instalado"
    echo -e "  ${YELLOW}→${NC} Execute: ${GREEN}./install_aguada.sh${NC}"
fi

# Resumo
echo -e "\n${BLUE}═════════════════════════════════════════${NC}"
echo -e "${BLUE}RESUMO:${NC}"
echo ""

if [ -f /etc/udev/rules.d/99-aguada-gateway.rules ] && [ -f /etc/systemd/system/aguada-autostart@.service ]; then
    echo -e "${GREEN}✓ Sistema de auto-start INSTALADO${NC}"
    echo ""
    echo "Para testar:"
    echo "  1. Conecte o gateway via USB"
    echo "  2. Sistema deve iniciar automaticamente"
    echo "  3. Dashboard abrirá no navegador"
else
    echo -e "${YELLOW}! Sistema de auto-start NÃO INSTALADO${NC}"
    echo ""
    echo "Para instalar:"
    echo -e "  ${GREEN}cd $PROJECT_DIR${NC}"
    echo -e "  ${GREEN}./install_aguada.sh${NC}"
fi

echo ""
echo "Ou inicie manualmente:"
echo -e "  ${GREEN}cd $PROJECT_DIR${NC}"
echo -e "  ${GREEN}./autostart_gateway.sh${NC}"

echo -e "\n${BLUE}═════════════════════════════════════════${NC}"
