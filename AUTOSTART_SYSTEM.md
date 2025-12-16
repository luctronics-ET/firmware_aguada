# 🚀 Sistema de Auto-Start AGUADA

Sistema automático que detecta conexão USB do gateway e inicia todo o ambiente.

## ✨ Funcionalidades

### 🔌 Detecção Automática
- Gateway conectado via USB → Sistema inicia automaticamente
- Detecta porta correta (`/dev/ttyACM*` ou `/dev/ttyUSB*`)
- Identifica dispositivo ESP32 via udev

### 📦 Instalação Inteligente
- Verifica dependências (Git, PHP, MySQL, ESP-IDF)
- Se sistema não instalado → Inicia instalador
- Se projeto não existe → Clona do GitHub
- Instala tudo automaticamente

### 🎯 Inicialização Completa
1. **MySQL** → Inicia banco de dados
2. **Backend PHP** → Servidor na porta 8080
3. **Gateway Monitor** → Terminal com logs ESP-NOW
4. **Dashboard** → Abre navegador automaticamente

---

## 📋 Instalação

### Método 1: Instalação Completa (Primeira vez)

```bash
cd ~/firmware_aguada
./install_aguada.sh
```

O instalador vai:
- ✅ Instalar dependências (Git, PHP, MySQL, Python, build-tools)
- ✅ Instalar ESP-IDF v5.1
- ✅ Configurar MySQL (criar banco `sensores_db`)
- ✅ Criar regras udev para auto-start
- ✅ Criar serviço systemd
- ✅ Configurar permissões USB
- ✅ Compilar firmware (opcional)

**⚠️ IMPORTANTE:** Após instalação, faça **logout e login** para aplicar permissões USB!

### Método 2: Instalação Manual de Componentes

#### 1. Criar regra udev:
```bash
sudo nano /etc/udev/rules.d/99-aguada-gateway.rules
```

Adicionar:
```udev
# AGUADA Gateway Auto-start
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"
```

Recarregar:
```bash
sudo udevadm control --reload-rules
```

#### 2. Criar serviço systemd:
```bash
sudo nano /etc/systemd/system/aguada-autostart@.service
```

Adicionar:
```ini
[Unit]
Description=AGUADA Gateway Auto-start Service
After=multi-user.target

[Service]
Type=oneshot
User=luciano
Environment="DISPLAY=:0"
Environment="DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus"
ExecStart=/home/luciano/firmware_aguada/autostart_gateway.sh
RemainAfterExit=no

[Install]
WantedBy=multi-user.target
```

Recarregar:
```bash
sudo systemctl daemon-reload
```

---

## 🎮 Uso

### Auto-start (Automático)
1. **Conecte gateway via USB**
2. Sistema detecta e inicia automaticamente:
   - 🔵 Notificação: "Sistema AGUADA iniciado"
   - 🖥️ Terminal abre com monitor do gateway
   - 🌐 Dashboard abre no navegador

### Manual
```bash
cd ~/firmware_aguada
./autostart_gateway.sh
```

---

## 📂 Arquivos Criados

```
firmware_aguada/
├── autostart_gateway.sh       ← Script principal de auto-start
├── install_aguada.sh          ← Instalador completo do sistema
│
/etc/udev/rules.d/
└── 99-aguada-gateway.rules    ← Regra para detectar gateway USB

/etc/systemd/system/
└── aguada-autostart@.service  ← Serviço que executa auto-start
```

---

## 🔧 Fluxo de Funcionamento

### 1. Gateway Conectado
```
USB plugged → udev detecta → systemd triggered → autostart_gateway.sh
```

### 2. Script Auto-start
```bash
autostart_gateway.sh:
├─ Verificar projeto existe? 
│  ├─ NÃO → Clonar GitHub → install_aguada.sh
│  └─ SIM → Continuar
│
├─ Verificar dependências?
│  ├─ Faltando → install_aguada.sh
│  └─ OK → Continuar
│
├─ Iniciar MySQL
├─ Iniciar Backend PHP (porta 8080)
├─ Detectar porta USB do gateway
├─ Abrir terminal com idf.py monitor
├─ Abrir navegador (dashboard.html)
└─ Mostrar notificação de sucesso
```

### 3. Instalador (se necessário)
```bash
install_aguada.sh:
├─ Detectar sistema operacional
├─ Instalar dependências (apt/dnf/pacman)
├─ Instalar ESP-IDF v5.1
├─ Configurar MySQL (criar banco)
├─ Criar regras udev
├─ Criar serviço systemd
├─ Adicionar usuário ao grupo dialout
└─ Compilar firmware (opcional)
```

---

## 🎯 Casos de Uso

### Caso 1: Primeira Instalação
```bash
# Clonar projeto
git clone https://github.com/SEU_USUARIO/firmware_aguada.git
cd firmware_aguada

# Instalar tudo
./install_aguada.sh

# Logout e login (para aplicar permissões)

# Conectar gateway → Sistema inicia automaticamente! ✨
```

### Caso 2: Sistema Já Instalado
```bash
# Apenas conectar gateway via USB
# → Auto-start detecta e inicia tudo automaticamente
```

### Caso 3: Iniciar Manualmente (sem conectar USB)
```bash
cd ~/firmware_aguada
./autostart_gateway.sh
```

---

## 🐛 Troubleshooting

### Problema: Auto-start não funciona

**1. Verificar se regra udev existe:**
```bash
ls -l /etc/udev/rules.d/99-aguada-gateway.rules
```

**2. Verificar se serviço systemd existe:**
```bash
systemctl status aguada-autostart@ttyACM0.service
```

**3. Ver logs do udev:**
```bash
sudo udevadm monitor
# Desconecte e reconecte gateway
```

**4. Verificar vendor ID do seu gateway:**
```bash
lsusb
# Procurar por ESP32 / CP2102 / CH340
```

**5. Testar manualmente:**
```bash
./autostart_gateway.sh
```

### Problema: Permissão negada em /dev/ttyACM0

```bash
# Adicionar usuário ao grupo dialout
sudo usermod -a -G dialout $USER

# Logout e login novamente!
```

### Problema: ESP-IDF não encontrado

```bash
# Instalar ESP-IDF
cd ~
mkdir -p esp
cd esp
git clone -b v5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32,esp32c3

# Adicionar ao bashrc
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc
source ~/.bashrc
```

### Problema: Backend não inicia

```bash
# Verificar se porta 8080 está livre
sudo lsof -i :8080

# Matar processos antigos
pkill -f "php -S localhost:8080"

# Iniciar manualmente
cd ~/firmware_aguada/backend
php -S localhost:8080
```

### Problema: MySQL não conecta

```bash
# Iniciar MySQL
sudo systemctl start mysql

# Verificar status
sudo systemctl status mysql

# Recriar banco
mysql -u root << EOF
CREATE DATABASE IF NOT EXISTS sensores_db;
CREATE USER IF NOT EXISTS 'aguada_user'@'localhost' IDENTIFIED BY '';
GRANT ALL PRIVILEGES ON sensores_db.* TO 'aguada_user'@'localhost';
FLUSH PRIVILEGES;
EOF
```

---

## 📊 Logs e Debugging

### Log do auto-start:
```bash
tail -f /tmp/aguada_autostart.log
```

### Log do backend:
```bash
tail -f /tmp/aguada_backend.log
```

### Monitor do gateway (manual):
```bash
source ~/esp/esp-idf/export.sh
cd ~/firmware_aguada/gateway_devkit_v1
idf.py -p /dev/ttyACM0 monitor
```

### Verificar leituras no banco:
```bash
cd ~/firmware_aguada/backend
php -r "
\$conn = new mysqli('localhost', 'aguada_user', '', 'sensores_db');
\$r = \$conn->query('SELECT * FROM leituras_v2 ORDER BY created_at DESC LIMIT 5');
while(\$row = \$r->fetch_assoc()) print_r(\$row);
"
```

---

## 🎨 Customizações

### Alterar porta do backend:
Editar `autostart_gateway.sh`:
```bash
# Linha 112: Alterar porta
php -S localhost:8080  # → php -S localhost:NOVA_PORTA
```

### Alterar URL do GitHub:
Editar `autostart_gateway.sh`:
```bash
# Linha 96: Alterar repositório
git clone https://github.com/SEU_USUARIO/firmware_aguada.git
```

### Desabilitar auto-start:
```bash
# Remover regra udev
sudo rm /etc/udev/rules.d/99-aguada-gateway.rules
sudo udevadm control --reload-rules
```

### Adicionar mais dispositivos USB:
Editar `/etc/udev/rules.d/99-aguada-gateway.rules`:
```udev
# Adicionar linha com vendor/product ID do seu dispositivo
SUBSYSTEM=="tty", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", TAG+="systemd", ENV{SYSTEMD_WANTS}="aguada-autostart@%k.service"
```

---

## 📚 Referências

- **ESP-IDF**: https://docs.espressif.com/projects/esp-idf/
- **udev rules**: https://wiki.archlinux.org/title/udev
- **systemd services**: https://www.freedesktop.org/software/systemd/man/systemd.service.html

---

## ✅ Checklist de Instalação

- [ ] Dependências instaladas (Git, PHP, MySQL, Python)
- [ ] ESP-IDF instalado em `~/esp/esp-idf`
- [ ] MySQL configurado (banco `sensores_db`)
- [ ] Regra udev criada em `/etc/udev/rules.d/`
- [ ] Serviço systemd criado em `/etc/systemd/system/`
- [ ] Usuário no grupo `dialout`
- [ ] **Logout e login feito** (importante!)
- [ ] Scripts executáveis (`chmod +x`)
- [ ] Gateway flashado com firmware atual
- [ ] Testado conectando gateway via USB

---

**Status:** ✅ Sistema pronto para uso!  
**Testado:** Ubuntu 24.04, ESP32 DevKit V1, ESP32-C3
