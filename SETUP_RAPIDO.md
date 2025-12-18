# 🚀 Guia Rápido - Setup do Sistema Aguada

## ⚠️ PROBLEMA: Autenticação MySQL

O MySQL está usando `auth_socket` (requer sudo). Temos 2 opções:

### Opção 1: Usar Scripts com sudo (Recomendado)

```bash
# Script completo (cria database automaticamente)
./start_complete_system.sh
# (vai pedir senha sudo para MySQL)
```

### Opção 2: Configurar Database Manualmente

Se preferir **não usar sudo** nos scripts:

#### 1. Setup Database (uma vez apenas)

```bash
# Criar database
sudo mysql -e "CREATE DATABASE IF NOT EXISTS aguada_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

# Importar schema
sudo mysql aguada_db < database/schema.sql

# Importar configuração dos nodes
sudo mysql aguada_db < firmware/nodes_config_REAL_MACS.sql

# Verificar
sudo mysql aguada_db -e "SELECT node_id, node_name, mac FROM node_configs;"
```

#### 2. Iniciar Sistema (sempre)

```bash
# Iniciar backend + frontend (sem sudo)
./start_frontend.sh
```

## 📊 URLs de Acesso

Após iniciar:
- **Backend:** http://192.168.0.117:8080
- **Frontend:** http://localhost:8080/aguada-telemetry.html

## 🔧 Comandos Úteis

```bash
# Ver status
./status_services.sh

# Parar tudo
./stop_services.sh

# Ver logs
tail -f server.log       # Backend
tail -f frontend.log     # Frontend
```

## 🐛 Troubleshooting

### Erro: "Access denied for user 'root'@'localhost'"

**Causa:** MySQL usando auth_socket (requer sudo)

**Solução 1 - Usar sudo:**
```bash
sudo mysql aguada_db < database/schema.sql
```

**Solução 2 - Criar usuário MySQL com senha:**
```bash
sudo mysql
```
```sql
CREATE USER 'aguada_user'@'localhost' IDENTIFIED BY 'aguada123';
GRANT ALL PRIVILEGES ON aguada_db.* TO 'aguada_user'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

Depois editar `backend/config.php`:
```php
$db_user = 'admin';
$db_pass = 'aguada123';
```

### Porta 8080 em uso

```bash
# Matar processos na porta
sudo lsof -ti:8080 | xargs kill -9

# Ou mudar porta no start_frontend.sh
# Editar: php -S 192.168.0.117:8081 -t backend
```

### Frontend não abre

```bash
cd frontend/aguada-dashboard

# Reinstalar dependências
rm -rf node_modules package-lock.json
npm install

# Tentar novamente
npm start
```

### Backend não recebe dados

```bash
# Verificar gateway
cd firmware/gateway_devkit_v1
idf.py monitor

# Verificar MySQL
sudo mysql aguada_db -e "SELECT COUNT(*) FROM leituras_v2;"

# Verificar API
curl http://192.168.0.117:8080/api/get_sensors_data.php
```

## ✅ Checklist de Inicialização

- [ ] MySQL rodando: `systemctl status mysql`
- [ ] Database criada: `sudo mysql -e "SHOW DATABASES;" | grep aguada_db`
- [ ] Schema importado: `sudo mysql aguada_db -e "SHOW TABLES;"`
- [ ] Nodes configurados: `sudo mysql aguada_db -e "SELECT COUNT(*) FROM node_configs;"`
- [ ] Backend rodando: `curl http://192.168.0.117:8080/api/get_sensors_data.php`
- [ ] Frontend instalado: `ls frontend/aguada-dashboard/node_modules`
- [ ] Frontend acessível: abrir http://localhost:8080/aguada-telemetry.html

## 🎯 Sequência Recomendada (Primeira Vez)

```bash
# 1. Setup database (uma vez, com sudo)
sudo mysql -e "CREATE DATABASE IF NOT EXISTS aguada_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
sudo mysql aguada_db < database/schema.sql
sudo mysql aguada_db < firmware/nodes_config_REAL_MACS.sql

# 2. Instalar frontend
cd frontend/aguada-dashboard
npm install
cd ../..

# 3. Iniciar sistema
./start_frontend.sh

# 4. Abrir navegador
firefox http://localhost:8080/aguada-telemetry.html
```

## 📱 Próximos Passos

Após sistema rodando:

1. ✅ Verificar se backend responde: `curl http://192.168.0.117:8080/api/get_sensors_data.php`
2. ✅ Abrir dashboard: http://localhost:8080/aguada-telemetry.html
3. ✅ Verificar se cards aparecem (podem estar offline se nodes não estiverem transmitindo)
4. ⏳ Conectar HC-SR04 nos nodes físicos
5. ⏳ Verificar gateway recebendo dados: `cd firmware/gateway_devkit_v1 && idf.py monitor`
6. ⏳ Validar dados chegando no dashboard

---

**Dica:** Use `./start_frontend.sh` para uso diário (mais rápido, sem sudo).  
Use `./start_complete_system.sh` apenas na primeira vez ou para recriar database.
