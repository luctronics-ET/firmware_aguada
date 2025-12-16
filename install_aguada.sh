#!/bin/bash
# Instalador completo do sistema AGUADA
# Detecta dependências faltando e instala automaticamente

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${CYAN}"
echo "===================================="
echo "   INSTALADOR AGUADA SYSTEM"
echo "===================================="
echo -e "${NC}"

log() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[!]${NC} $1"
}

ask() {
    echo -e "${BLUE}[?]${NC} $1"
}

# ============================================
# 1. VERIFICAR SISTEMA OPERACIONAL
# ============================================

echo ""
ask "Detectando sistema operacional..."

if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VER=$VERSION_ID
    log "Sistema: $PRETTY_NAME"
else
    log_error "Sistema não identificado"
    exit 1
fi

# ============================================
# 2. INSTALAR DEPENDÊNCIAS BASE
# ============================================

echo ""
ask "Instalando dependências base..."

install_deps() {
    case $OS in
        ubuntu|debian|linuxmint)
            sudo apt-get update
            sudo apt-get install -y \
                git curl wget \
                python3 python3-pip python3-venv \
                php php-cli php-mysql \
                mysql-server \
                build-essential cmake ninja-build \
                libusb-1.0-0-dev libudev-dev \
                ccache dfu-util \
                libnotify-bin zenity \
                screen tmux
            ;;
        fedora|rhel|centos)
            sudo dnf install -y \
                git curl wget \
                python3 python3-pip \
                php php-mysqlnd \
                mysql-server \
                gcc gcc-c++ make cmake ninja-build \
                libusb-devel libudev-devel \
                ccache dfu-util \
                libnotify zenity \
                screen tmux
            ;;
        arch|manjaro)
            sudo pacman -S --noconfirm \
                git curl wget \
                python python-pip \
                php php-mysql \
                mariadb \
                base-devel cmake ninja \
                libusb libudev0-shim \
                ccache dfu-util \
                libnotify zenity \
                screen tmux
            ;;
        *)
            log_error "Sistema $OS não suportado automaticamente"
            log_warn "Instale manualmente: git, python3, php, mysql, build-essentials"
            exit 1
            ;;
    esac
}

install_deps
log "Dependências base instaladas"

# ============================================
# 3. INSTALAR ESP-IDF
# ============================================

echo ""
ask "Instalando ESP-IDF..."

if [ ! -d "$HOME/esp/esp-idf" ]; then
    mkdir -p "$HOME/esp"
    cd "$HOME/esp"
    
    log "Clonando ESP-IDF v5.1..."
    git clone -b v5.1 --recursive https://github.com/espressif/esp-idf.git
    
    cd esp-idf
    log "Instalando ferramentas ESP-IDF..."
    ./install.sh esp32,esp32c3
    
    log "✓ ESP-IDF instalado em $HOME/esp/esp-idf"
else
    log "ESP-IDF já instalado"
fi

# Adicionar ao bashrc
if ! grep -q "esp-idf/export.sh" "$HOME/.bashrc"; then
    echo "" >> "$HOME/.bashrc"
    echo "# ESP-IDF" >> "$HOME/.bashrc"
    echo "alias get_idf='. \$HOME/esp/esp-idf/export.sh'" >> "$HOME/.bashrc"
    log "Alias 'get_idf' adicionado ao .bashrc"
fi

# ============================================
# 4. CONFIGURAR MYSQL
# ============================================

echo ""
ask "Configurando MySQL..."

# Iniciar MySQL
sudo systemctl start mysql
sudo systemctl enable mysql

# Criar banco e usuário
log "Criando banco de dados 'sensores_db'..."

mysql -u root << EOF
CREATE DATABASE IF NOT EXISTS sensores_db;
CREATE USER IF NOT EXISTS 'aguada_user'@'localhost' IDENTIFIED BY '';
GRANT ALL PRIVILEGES ON sensores_db.* TO 'aguada_user'@'localhost';
FLUSH PRIVILEGES;
EOF

# Importar schema
if [ -f "$PROJECT_DIR/backend/schema.sql" ]; then
    log "Importando schema..."
    mysql -u aguada_user sensores_db < "$PROJECT_DIR/backend/schema.sql"
elif [ -f "$PROJECT_DIR/database/schema.sql" ]; then
    log "Importando schema..."
    mysql -u aguada_user sensores_db < "$PROJECT_DIR/database/schema.sql"
fi

log "✓ MySQL configurado"

# ============================================
# 5. CONFIGURAR PERMISSÕES USB
# ============================================

echo ""
ask "Configurando permissões USB..."

# Adicionar usuário ao grupo dialout
sudo usermod -a -G dialout $USER

log "✓ Usuário adicionado ao grupo 'dialout'"
log_warn "IMPORTANTE: Faça logout e login novamente para aplicar permissões USB"

# ============================================
# 6. CRIAR UDEV RULE PARA AUTO-START
# ============================================

echo ""
ask "Configurando auto-start ao conectar gateway..."

sudo tee /etc/udev/rules.d/99-aguada-gateway.rules > /dev/null << EOF
# AGUADA Gateway Auto-start
# Triggers quando ESP32 é conectado via USB

# ESP32 DevKit (CP2102)
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"

# ESP32 DevKit (CH340)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"

# ESP32-C3 (USB Serial/JTAG)
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"

# Qualquer CP210x (fallback)
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"
EOF

sudo udevadm control --reload-rules
log "✓ Regra udev criada"

# ============================================
# 7. CRIAR SYSTEMD SERVICE
# ============================================

echo ""
ask "Criando serviço systemd..."

sudo tee /etc/systemd/system/aguada-autostart@.service > /dev/null << EOF
[Unit]
Description=AGUADA Gateway Auto-start Service
After=multi-user.target

[Service]
Type=oneshot
User=$USER
Environment="DISPLAY=:0"
Environment="DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u $USER)/bus"
ExecStart=$PROJECT_DIR/autostart_gateway.sh
RemainAfterExit=no

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
log "✓ Serviço systemd criado"

# ============================================
# 8. TORNAR SCRIPTS EXECUTÁVEIS
# ============================================

echo ""
ask "Configurando permissões de scripts..."

chmod +x "$PROJECT_DIR/autostart_gateway.sh"
chmod +x "$PROJECT_DIR/install_aguada.sh"

if [ -f "$PROJECT_DIR/start_services.sh" ]; then
    chmod +x "$PROJECT_DIR/start_services.sh"
fi

if [ -f "$PROJECT_DIR/stop_services.sh" ]; then
    chmod +x "$PROJECT_DIR/stop_services.sh"
fi

log "✓ Scripts configurados"

# ============================================
# 9. COMPILAR FIRMWARE (OPCIONAL)
# ============================================

echo ""
read -p "$(echo -e ${BLUE}[?]${NC} Deseja compilar o firmware agora? [s/N]:) " -n 1 -r
echo

if [[ $REPLY =~ ^[SsYy]$ ]]; then
    log "Compilando gateway..."
    
    source "$HOME/esp/esp-idf/export.sh"
    
    cd "$PROJECT_DIR/gateway_devkit_v1"
    idf.py build
    
    log "✓ Gateway compilado"
    
    read -p "$(echo -e ${BLUE}[?]${NC} Deseja flashar o gateway agora? [s/N]:) " -n 1 -r
    echo
    
    if [[ $REPLY =~ ^[SsYy]$ ]]; then
        log "Procurando porta USB..."
        
        for port in /dev/ttyUSB0 /dev/ttyACM0; do
            if [ -e "$port" ]; then
                log "Flashando em $port..."
                idf.py -p "$port" flash
                log "✓ Gateway flashado"
                break
            fi
        done
    fi
fi

# ============================================
# 10. FINALIZAÇÃO
# ============================================

echo ""
echo -e "${GREEN}"
echo "===================================="
echo "   INSTALAÇÃO CONCLUÍDA!"
echo "===================================="
echo -e "${NC}"
echo ""
log "Sistema AGUADA instalado com sucesso!"
echo ""
echo -e "${CYAN}Próximos passos:${NC}"
echo ""
echo "1. ${YELLOW}Faça logout e login${NC} para aplicar permissões USB"
echo ""
echo "2. Conecte o gateway via USB:"
echo "   → Sistema iniciará automaticamente"
echo "   → Dashboard abrirá no navegador"
echo ""
echo "3. Ou inicie manualmente:"
echo "   ${GREEN}$PROJECT_DIR/autostart_gateway.sh${NC}"
echo ""
echo "4. Para compilar firmware:"
echo "   ${GREEN}get_idf${NC}  # Carregar ESP-IDF"
echo "   ${GREEN}cd $PROJECT_DIR/gateway_devkit_v1${NC}"
echo "   ${GREEN}idf.py build flash monitor${NC}"
echo ""
echo -e "${CYAN}URLs úteis:${NC}"
echo "   Dashboard: ${BLUE}http://localhost:8080/dashboard.html${NC}"
echo "   SCADA:     ${BLUE}http://localhost:8080/scada.html${NC}"
echo "   Mapa:      ${BLUE}http://localhost:8080/mapa.html${NC}"
echo ""
log "Documentação completa em: $PROJECT_DIR/README.md"
echo ""
