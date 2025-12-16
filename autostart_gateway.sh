#!/bin/bash
# Auto-start script for AGUADA Gateway
# Triggers when ESP32 gateway is connected via USB

set -e

# ============================================
# CONFIGURAÇÕES
# ============================================
PROJECT_DIR="/home/luciano/firmware_aguada"
BACKEND_DIR="$PROJECT_DIR/backend"
GATEWAY_DIR="$PROJECT_DIR/firmware/gateway_devkit_v1"
NODE_DIR="$PROJECT_DIR/firmware/node_ultra1"
INSTALL_FLAG="/tmp/aguada_installing"
LOG_FILE="/tmp/aguada_autostart.log"

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================
# FUNÇÕES
# ============================================

log() {
    echo -e "${GREEN}[AGUADA]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERRO]${NC} $1" | tee -a "$LOG_FILE"
}

log_warn() {
    echo -e "${YELLOW}[AVISO]${NC} $1" | tee -a "$LOG_FILE"
}

show_notification() {
    if command -v notify-send &> /dev/null; then
        notify-send "AGUADA System" "$1" -i network-server -t 5000
    fi
}

check_dependencies() {
    log "Verificando dependências..."
    
    local missing=()
    
    # Verificar comandos essenciais
    command -v git &> /dev/null || missing+=("git")
    command -v php &> /dev/null || missing+=("php")
    command -v mysql &> /dev/null || missing+=("mysql-server")
    command -v python3 &> /dev/null || missing+=("python3")
    
    # Verificar ESP-IDF
    if [ ! -d "$HOME/esp/esp-idf" ]; then
        missing+=("esp-idf")
    fi
    
    if [ ${#missing[@]} -gt 0 ]; then
        log_warn "Dependências faltando: ${missing[*]}"
        return 1
    fi
    
    log "✓ Todas as dependências OK"
    return 0
}

install_system() {
    if [ -f "$INSTALL_FLAG" ]; then
        log "Instalação já em andamento..."
        return 1
    fi
    
    touch "$INSTALL_FLAG"
    
    log "===================================="
    log "  INSTALAÇÃO AGUADA SYSTEM"
    log "===================================="
    show_notification "Iniciando instalação automática..."
    
    # Abrir terminal com instalação
    if command -v gnome-terminal &> /dev/null; then
        gnome-terminal -- bash -c "
            cd '$PROJECT_DIR'
            ./install_aguada.sh
            read -p 'Pressione ENTER para fechar...'
        "
    elif command -v xterm &> /dev/null; then
        xterm -e "cd '$PROJECT_DIR' && ./install_aguada.sh && read -p 'Pressione ENTER...'"
    fi
    
    rm -f "$INSTALL_FLAG"
}

check_project_exists() {
    if [ ! -d "$PROJECT_DIR" ]; then
        log_warn "Projeto não encontrado em $PROJECT_DIR"
        
        show_notification "Projeto não encontrado! Iniciando clone do GitHub..."
        
        # Clone do repositório
        log "Clonando repositório do GitHub..."
        cd ~
        git clone https://github.com/SEU_USUARIO/firmware_aguada.git
        
        if [ $? -ne 0 ]; then
            log_error "Falha ao clonar repositório"
            show_notification "Erro ao clonar repositório do GitHub"
            exit 1
        fi
        
        log "✓ Repositório clonado"
        show_notification "Repositório clonado! Iniciando instalação..."
        
        return 1  # Precisa instalar
    fi
    
    return 0  # Projeto já existe
}

start_mysql() {
    log "Verificando MySQL..."
    
    if ! systemctl is-active --quiet mysql; then
        log "Iniciando MySQL..."
        sudo systemctl start mysql
        sleep 2
    fi
    
    log "✓ MySQL rodando"
}

start_backend() {
    log "Verificando backend PHP..."
    
    # Matar processos antigos
    pkill -f "php -S localhost:8080" 2>/dev/null || true
    
    cd "$BACKEND_DIR"
    
    # Iniciar servidor PHP em background
    php -S localhost:8080 > /tmp/aguada_backend.log 2>&1 &
    
    sleep 2
    
    # Verificar se iniciou
    if curl -s http://localhost:8080 > /dev/null 2>&1; then
        log "✓ Backend PHP rodando na porta 8080"
        return 0
    else
        log_error "Falha ao iniciar backend"
        return 1
    fi
}

find_gateway_port() {
    log "Procurando gateway USB..."
    
    # Procurar por ESP32
    for port in /dev/ttyUSB* /dev/ttyACM*; do
        if [ -e "$port" ]; then
            # Verificar se é ESP32 (via dmesg ou udevadm)
            local device_info=$(udevadm info -a -n "$port" 2>/dev/null | grep -i "esp32\|cp210\|ch340\|ftdi" || true)
            
            if [ -n "$device_info" ]; then
                log "✓ Gateway encontrado em $port"
                echo "$port"
                return 0
            fi
        fi
    done
    
    # Se não encontrou via udev, tentar primeira porta disponível
    for port in /dev/ttyACM0 /dev/ttyUSB0; do
        if [ -e "$port" ]; then
            log_warn "Usando $port (não confirmado se é gateway)"
            echo "$port"
            return 0
        fi
    done
    
    log_error "Nenhuma porta USB encontrada"
    return 1
}

start_gateway_monitor() {
    local port=$1
    
    log "Iniciando monitor do gateway em $port..."
    
    # Verificar se diretório do gateway existe
    if [ ! -d "$GATEWAY_DIR" ]; then
        log_error "Diretório do gateway não encontrado: $GATEWAY_DIR"
        return 1
    fi
    
    # Verificar se ESP-IDF está configurado
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        source "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
    else
        log_error "ESP-IDF não encontrado"
        return 1
    fi
    
    # Abrir terminal com monitor
    if command -v gnome-terminal &> /dev/null; then
        gnome-terminal --title="AGUADA Gateway Monitor" -- bash -c "
            source $HOME/esp/esp-idf/export.sh
            cd '$GATEWAY_DIR'
            echo '===================================='
            echo '  AGUADA GATEWAY MONITOR'
            echo '===================================='
            echo ''
            idf.py -p $port monitor
        " &
    elif command -v xterm &> /dev/null; then
        xterm -T "AGUADA Gateway Monitor" -e "
            source $HOME/esp/esp-idf/export.sh
            cd '$GATEWAY_DIR'
            idf.py -p $port monitor
        " &
    fi
    
    log "✓ Monitor do gateway iniciado"
}

open_dashboard() {
    log "Aguardando backend estabilizar..."
    sleep 3
    
    log "Abrindo dashboard no navegador..."
    
    # Tentar abrir no navegador padrão
    if command -v xdg-open &> /dev/null; then
        xdg-open "http://localhost:8080/dashboard.html" &
    elif command -v gnome-open &> /dev/null; then
        gnome-open "http://localhost:8080/dashboard.html" &
    elif command -v firefox &> /dev/null; then
        firefox "http://localhost:8080/dashboard.html" &
    fi
    
    log "✓ Dashboard aberto"
    show_notification "Sistema AGUADA iniciado! Dashboard aberto no navegador."
}

show_status_window() {
    if command -v zenity &> /dev/null; then
        zenity --info \
            --title="AGUADA System" \
            --text="<b>Sistema AGUADA Iniciado!</b>

✓ Gateway: Monitorando em tempo real
✓ Backend: http://localhost:8080
✓ MySQL: Banco de dados ativo
✓ Dashboard: Aberto no navegador

<i>Verifique o terminal do gateway para ver leituras dos sensores.</i>" \
            --width=400 &
    fi
}

# ============================================
# MAIN
# ============================================

main() {
    log "===================================="
    log "  AGUADA AUTO-START"
    log "  $(date '+%Y-%m-%d %H:%M:%S')"
    log "===================================="
    
    # 1. Verificar se projeto existe
    if ! check_project_exists; then
        # Projeto foi clonado, precisa instalar
        install_system
        exit 0
    fi
    
    # 2. Verificar dependências
    if ! check_dependencies; then
        log_warn "Sistema não está completamente instalado"
        
        if [ -f "$PROJECT_DIR/install_aguada.sh" ]; then
            log "Iniciando instalador..."
            install_system
        else
            log_error "Script de instalação não encontrado"
            show_notification "Erro: Sistema precisa ser instalado manualmente"
        fi
        
        exit 1
    fi
    
    # 3. Iniciar MySQL
    start_mysql || {
        log_error "Falha ao iniciar MySQL"
        show_notification "Erro ao iniciar MySQL"
        exit 1
    }
    
    # 4. Iniciar backend PHP
    start_backend || {
        log_error "Falha ao iniciar backend"
        show_notification "Erro ao iniciar backend PHP"
        exit 1
    }
    
    # 5. Encontrar porta do gateway
    gateway_port=$(find_gateway_port)
    if [ -z "$gateway_port" ]; then
        log_error "Gateway não encontrado"
        show_notification "Gateway USB não detectado"
        
        # Abrir dashboard mesmo assim
        open_dashboard
        exit 1
    fi
    
    # 6. Iniciar monitor do gateway
    start_gateway_monitor "$gateway_port"
    
    # 7. Abrir dashboard
    open_dashboard
    
    # 8. Mostrar janela de status
    show_status_window
    
    log "===================================="
    log "  SISTEMA INICIADO COM SUCESSO!"
    log "===================================="
    log ""
    log "Gateway: $gateway_port"
    log "Backend: http://localhost:8080"
    log "Dashboard: http://localhost:8080/dashboard.html"
    log ""
}

# Executar
main "$@"
